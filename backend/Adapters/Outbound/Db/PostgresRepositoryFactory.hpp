#ifndef HEADER__FILE__POSTGRESREPOSITORYFACTORY
#define HEADER__FILE__POSTGRESREPOSITORYFACTORY

#include <cstddef>
#include <memory>
#include <string>

#include <boost/asio/any_io_executor.hpp>

class IAccountRepository;

struct PostgresDbConfig
{
    std::string address;
    std::string port;
    std::string name;
    std::string user;
    std::string password;
    std::size_t pool_size = 0;
};

class PostgresRepositoryFactory
{
  public:
    static std::shared_ptr<IAccountRepository> CreateAccountRepository(const PostgresDbConfig &config,
                                                                       boost::asio::any_io_executor db_exec);
};

#endif /* HEADER__FILE__POSTGRESREPOSITORYFACTORY */
