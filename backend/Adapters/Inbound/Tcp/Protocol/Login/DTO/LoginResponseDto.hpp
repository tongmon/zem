#ifndef HEADER__FILE__LOGINRESPONSEDTO
#define HEADER__FILE__LOGINRESPONSEDTO

#include "Domain/LoginResult.hpp"

struct LoginResponseDto
{
    LoginResult result_code = LoginResult::AccountLocked;
};

#endif /* HEADER__FILE__LOGINRESPONSEDTO */
