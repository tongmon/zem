#ifndef HEADER__FILE__ILOGINUSECASE
#define HEADER__FILE__ILOGINUSECASE

#include <string_view>

#include "Application/Common/Task.hpp"
#include "Domain/LoginResult.hpp"

class ILoginUseCase
{
  public:
    virtual ~ILoginUseCase() = default;
    virtual Task<LoginResult> ValidateCredentials(std::string_view id, std::string_view password) = 0;
};

#endif /* HEADER__FILE__ILOGINUSECASE */
