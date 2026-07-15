#ifndef HEADER__FILE__POSTGRESACCOUNTREPOSITORY
#define HEADER__FILE__POSTGRESACCOUNTREPOSITORY

#include <memory>
#include <string>
#include <utility>

#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/awaitable.hpp>
#include <soci/postgresql/soci-postgresql.h>
#include <soci/soci.h>

#include "Application/Ports/Outbound/IAccountRepository.hpp"

class PostgresAccountRepository : public IAccountRepository
{
  public:
    PostgresAccountRepository(std::shared_ptr<soci::connection_pool> pool,
                              boost::asio::any_io_executor db_exec);

    // Task<LoginResult> ValidateCredentials(std::string id, std::string password) override;

    boost::asio::awaitable<std::optional<std::string>> GetPassword(std::string_view id) override;

  private:
    std::shared_ptr<soci::connection_pool> m_pool;
    boost::asio::any_io_executor m_db_exec;
};

#endif /* HEADER__FILE__POSTGRESACCOUNTREPOSITORY */
