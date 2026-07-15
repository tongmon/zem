#ifndef HEADER__FILE__APPCONTEXT
#define HEADER__FILE__APPCONTEXT

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

namespace boost::asio
{
class thread_pool;
}

class TCPServer;
class LoginUseCase;
class IAccountRepository;

class AppContext
{
  public:
    struct DbConfig
    {
        std::string address;
        std::string port;
        std::string name;
        std::string user;
        std::string password;
    };

    struct Config
    {
        DbConfig db;
        std::uint16_t port = 0;
        std::size_t db_pool_size = 0;
        std::size_t db_thread_count = 0;
    };

    static std::unique_ptr<AppContext> CreateDefault();

    explicit AppContext(Config config);
    ~AppContext();

    AppContext(const AppContext &) = delete;
    AppContext &operator=(const AppContext &) = delete;
    AppContext(AppContext &&) = delete;
    AppContext &operator=(AppContext &&) = delete;

    void Run();

  private:
    void InitLogger();
    void InitDb();
    void BuildUseCases();
    void BuildServer();
    void ShutdownLogger();
    void StopDbThreadPool();

    Config config_;
    std::unique_ptr<boost::asio::thread_pool> db_thread_pool_;
    std::shared_ptr<IAccountRepository> account_repo_;
    std::shared_ptr<LoginUseCase> login_usecase_;
    std::shared_ptr<TCPServer> server_;
    bool logger_initialized_ = false;
    bool db_pool_stopped_ = false;
};

#endif /* HEADER__FILE__APPCONTEXT */
