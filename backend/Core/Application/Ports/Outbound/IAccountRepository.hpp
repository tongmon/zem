#ifndef HEADER__FILE__IACCOUNTREPOSITORY
#define HEADER__FILE__IACCOUNTREPOSITORY

#include <optional>
#include <string>

#include "Application/Common/Task.hpp"
#include "Domain/LoginResult.hpp"

class IAccountRepository
{
  public:
    virtual ~IAccountRepository() = default;
    virtual Task<std::optional<std::string>> GetPassword(std::string_view id) = 0;
    // virtual boost::asio::awaitable<LoginResult> ValidateCredentials(std::string id, std::string password) = 0;
};

#endif /* HEADER__FILE__IACCOUNTREPOSITORY */
