#ifndef CORE_DOMAIN_LOGINRESULT_HPP
#define CORE_DOMAIN_LOGINRESULT_HPP

#include <cstdint>

enum class LoginResult : std::uint32_t
{
    Success = 0,
    InvalidCredentials,
    AccountLocked,
};

#endif /* CORE_DOMAIN_LOGINRESULT_HPP */
