#include "PostgresRepositoryFactory.hpp"

#include "Application/Ports/Outbound/IAccountRepository.hpp"
#include "PostgresAccountRepository.hpp"

#include <format>
#include <thread>

#include <soci/postgresql/soci-postgresql.h>
#include <soci/soci.h>

std::shared_ptr<IAccountRepository> PostgresRepositoryFactory::CreateAccountRepository(const PostgresDbConfig &config,
                                                                                        boost::asio::any_io_executor db_exec)
{
    std::size_t concurrency = std::thread::hardware_concurrency() ? std::thread::hardware_concurrency() : 1;
    std::size_t pool_size = config.pool_size ? config.pool_size : 2 * concurrency;

    auto pool = std::make_shared<soci::connection_pool>(pool_size);
    for (std::size_t i = 0; i < pool_size; ++i)
    {
        soci::session &sql = pool->at(i);
        sql.open(soci::postgresql,
                 std::format("host={} port={} dbname={} user={} password={}",
                             config.address,
                             config.port,
                             config.name,
                             config.user,
                             config.password));
    }

    return std::make_shared<PostgresAccountRepository>(pool, std::move(db_exec));
}
