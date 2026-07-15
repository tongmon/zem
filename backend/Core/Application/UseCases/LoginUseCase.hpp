#ifndef HEADER__FILE__LOGINUSECASE
#define HEADER__FILE__LOGINUSECASE

#include <memory>
#include <string>

#include "Application/Ports/Inbound/ILoginUseCase.hpp"
#include "Application/Ports/Outbound/IAccountRepository.hpp"

class LoginUseCase : public ILoginUseCase
{
  public:
    explicit LoginUseCase(std::shared_ptr<IAccountRepository> repo);
    Task<LoginResult> ValidateCredentials(std::string_view id, std::string_view password) override;

  private:
    std::shared_ptr<IAccountRepository> m_repo;
};

#endif /* HEADER__FILE__LOGINUSECASE */
