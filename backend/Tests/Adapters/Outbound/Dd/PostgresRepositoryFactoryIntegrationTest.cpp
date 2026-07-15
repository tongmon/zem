#include "PostgresRepositoryFactory.hpp"
#include "Application/Ports/Outbound/IAccountRepository.hpp"

#include <chrono>
#include <cstdlib>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/asio/use_future.hpp>
#include <gtest/gtest.h>
#include <soci/postgresql/soci-postgresql.h>
#include <soci/soci.h>

namespace
{
const char *GetEnv(const char *name)
{
    const char *value = std::getenv(name);
    if (!value || !*value)
        return nullptr;
    return value;
}

bool IsIntegrationEnabled()
{
    const char *flag = GetEnv("ZEM_TEST_DB_INTEGRATION");
    return flag && std::string_view(flag) == "1";
}

std::optional<PostgresDbConfig> ReadDbConfigFromEnv()
{
    PostgresDbConfig config;

    const char *address = GetEnv("ZEM_TEST_DB_ADDRESS");
    const char *port = GetEnv("ZEM_TEST_DB_PORT");
    const char *name = GetEnv("ZEM_TEST_DB_NAME");
    const char *user = GetEnv("ZEM_TEST_DB_USER");
    const char *password = GetEnv("ZEM_TEST_DB_PASSWORD");

    if (!address || !port || !name || !user || !password)
        return std::nullopt;

    config.address = address;
    config.port = port;
    config.name = name;
    config.user = user;
    config.password = password;
    config.pool_size = 2;
    return config;
}

std::string BuildConnectionString(const PostgresDbConfig &config)
{
    return std::string("host=") + config.address +
           " port=" + config.port +
           " dbname=" + config.name +
           " user=" + config.user +
           " password=" + config.password;
}

std::string MakeTestAccountId()
{
    return "zem_test_user_" + std::to_string(static_cast<unsigned long long>(
                                  std::chrono::high_resolution_clock::now().time_since_epoch().count()));
}

template <typename T>
T RunTask(boost::asio::awaitable<T> task)
{
    boost::asio::io_context io;
    auto future = boost::asio::co_spawn(io, std::move(task), boost::asio::use_future);
    io.run();
    return future.get();
}
} // namespace

TEST(PostgresRepositoryFactoryIntegrationTest, CreateRepositoryAndFetchPasswordFromRealDb)
{
    if (!IsIntegrationEnabled())
        GTEST_SKIP() << "Set ZEM_TEST_DB_INTEGRATION=1 to run DB integration tests.";

    auto config_opt = ReadDbConfigFromEnv();
    if (!config_opt)
        GTEST_SKIP() << "Set ZEM_TEST_DB_ADDRESS/PORT/NAME/USER/PASSWORD for DB integration tests.";

    const PostgresDbConfig config = *config_opt;
    const std::string connection_string = BuildConnectionString(config);

    soci::session setup_sql;
    try
    {
        setup_sql.open(soci::postgresql, connection_string);
    }
    catch (const std::exception &e)
    {
        FAIL() << "Failed to connect test DB: " << e.what();
        return;
    }

    // Use dedicated test DB; this table shape is required by PostgresAccountRepository query.
    setup_sql << "create table if not exists account (id text primary key, password text not null)";

    const std::string id = MakeTestAccountId();
    const std::string password = "pw_from_real_db";

    setup_sql << "insert into account(id, password) values(:id, :password) "
                 "on conflict (id) do update set password = excluded.password",
        soci::use(id), soci::use(password);

    boost::asio::thread_pool db_pool(1);
    std::shared_ptr<IAccountRepository> repo;
    try
    {
        repo = PostgresRepositoryFactory::CreateAccountRepository(config, db_pool.get_executor());
    }
    catch (const std::exception &e)
    {
        db_pool.stop();
        db_pool.join();
        FAIL() << "Factory failed to create repository: " << e.what();
        return;
    }

    auto got_password = RunTask(repo->GetPassword(id));
    auto missing_password = RunTask(repo->GetPassword("this_user_should_not_exist"));

    EXPECT_TRUE(got_password.has_value());
    EXPECT_EQ(got_password.value_or(""), password);
    EXPECT_FALSE(missing_password.has_value());

    setup_sql << "delete from account where id=:id", soci::use(id);

    db_pool.stop();
    db_pool.join();
}
