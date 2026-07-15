#include "LoginUseCase.hpp"

#include <utility>

LoginUseCase::LoginUseCase(std::shared_ptr<IAccountRepository> repo)
    : m_repo{std::move(repo)}
{
}

Task<LoginResult> LoginUseCase::ValidateCredentials(std::string_view id, std::string_view password)
{
    LoginResult ret = LoginResult::AccountLocked;
    auto stored_pw = co_await m_repo->GetPassword(id);
    if (stored_pw.has_value() && stored_pw.value() == password)
        ret = LoginResult::Success;
    else
        ret = LoginResult::InvalidCredentials;

    co_return ret;
}
