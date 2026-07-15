#ifndef HEADER__FILE__LOGINRESPONSEDTOV1
#define HEADER__FILE__LOGINRESPONSEDTOV1

#include "Domain/LoginResult.hpp"

namespace V1
{
struct LoginResponseDto
{
    LoginResult result_code = LoginResult::AccountLocked;
};
} // namespace V1

#endif /* HEADER__FILE__LOGINRESPONSEDTOV1 */
