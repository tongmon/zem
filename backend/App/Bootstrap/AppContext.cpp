#include "AppContext.hpp"

#include "Application/UseCases/LoginUseCase.hpp"
#include "PostgresRepositoryFactory.hpp"
#include "Server/TCPServer.hpp"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <format>
#include <limits>
#include <thread>

#include <boost/asio/thread_pool.hpp>
#include <boost/dll.hpp>
#include <boost/system/system_error.hpp>
#include <spdlog/async.h>
#include <spdlog/async_logger.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/spdlog.h>

namespace
{
const char *GetEnv(const char *name)
{
    const char *value = std::getenv(name);
    if (!value || !*value)
        return nullptr;
    return value;
}

std::string GetEnvOrDefault(const char *name, std::string default_value)
{
    if (const char *value = GetEnv(name))
        return std::string(value);
    return default_value;
}

std::size_t GetEnvOrDefaultSize(const char *name, std::size_t default_value)
{
    if (const char *value = GetEnv(name))
    {
        char *end = nullptr;
        unsigned long parsed = std::strtoul(value, &end, 10);
        if (end != value && *end == '\0')
            return static_cast<std::size_t>(parsed);
    }
    return default_value;
}

std::uint16_t GetEnvOrDefaultPort(const char *name, std::uint16_t default_value)
{
    if (const char *value = GetEnv(name))
    {
        char *end = nullptr;
        unsigned long parsed = std::strtoul(value, &end, 10);
        if (end != value && *end == '\0' && parsed <= std::numeric_limits<std::uint16_t>::max())
            return static_cast<std::uint16_t>(parsed);
    }
    return default_value;
}
} // namespace

std::unique_ptr<AppContext> AppContext::CreateDefault()
{
    Config config;
    config.db.address = GetEnvOrDefault("ZEM_DB_ADDRESS", "127.0.0.1");
    config.db.port = GetEnvOrDefault("ZEM_DB_PORT", "5432");
    config.db.name = GetEnvOrDefault("ZEM_DB_NAME", "wms_db");
    config.db.user = GetEnvOrDefault("ZEM_DB_USER", "wms_manager");
    config.db.password = GetEnvOrDefault("ZEM_DB_PASSWORD", "kyungjoon_1997");
    config.port = GetEnvOrDefaultPort("ZEM_PORT", 47213);
    config.db_pool_size = GetEnvOrDefaultSize("ZEM_DB_POOL_SIZE", 0);
    config.db_thread_count = GetEnvOrDefaultSize("ZEM_DB_THREAD_COUNT", 0);

    return std::make_unique<AppContext>(std::move(config));
}

AppContext::AppContext(Config config)
    : config_(std::move(config))
{
    InitLogger();
    InitDb();
    BuildUseCases();
    BuildServer();
}

AppContext::~AppContext()
{
    StopDbThreadPool();
    server_.reset();
    ShutdownLogger();
}

void AppContext::Run()
{
    if (!server_)
        return;

    server_->Run();
}

void AppContext::InitLogger()
{
    if (logger_initialized_)
        return;

    std::string log_path = std::format("{}/Log/zem_{:%Y%m%dT%H%M%S}.log",
                                       boost::dll::program_location().parent_path().string(),
                                       std::chrono::system_clock::now());
    int max_file_size = 1024 * 1024 * 5; // 5MB
    int max_file_num = 5;                // 5 maximum log file number

    auto file_logger = spdlog::create_async<spdlog::sinks::rotating_file_sink_mt>(
        "zem_file_logger",
        log_path,
        max_file_size,
        max_file_num);
    file_logger->flush_on(spdlog::level::err);

    spdlog::flush_every(std::chrono::seconds(3));
    spdlog::set_default_logger(file_logger);
    spdlog::set_pattern("[%H:%M:%S %z] [%t] [%^-%L-%$] %v");
    spdlog::set_level(spdlog::level::trace);

    logger_initialized_ = true;
}

void AppContext::InitDb()
{
    std::size_t concurrency = std::thread::hardware_concurrency() ? std::thread::hardware_concurrency() : 1;

    std::size_t thread_count = config_.db_thread_count ? config_.db_thread_count : std::max<std::size_t>(2, concurrency);
    db_thread_pool_ = std::make_unique<boost::asio::thread_pool>(static_cast<unsigned int>(thread_count));

    PostgresDbConfig db_config;
    db_config.address = config_.db.address;
    db_config.port = config_.db.port;
    db_config.name = config_.db.name;
    db_config.user = config_.db.user;
    db_config.password = config_.db.password;
    db_config.pool_size = config_.db_pool_size;
    account_repo_ = PostgresRepositoryFactory::CreateAccountRepository(db_config, db_thread_pool_->get_executor());
}

void AppContext::BuildUseCases()
{
    login_usecase_ = std::make_shared<LoginUseCase>(account_repo_);
}

void AppContext::BuildServer()
{
    server_ = std::make_shared<TCPServer>(login_usecase_, config_.port);
}

void AppContext::ShutdownLogger()
{
    if (!logger_initialized_)
        return;

    spdlog::shutdown();
    logger_initialized_ = false;
}

void AppContext::StopDbThreadPool()
{
    if (db_pool_stopped_ || !db_thread_pool_)
        return;

    db_thread_pool_->stop();
    db_thread_pool_->join();
    db_pool_stopped_ = true;
}
