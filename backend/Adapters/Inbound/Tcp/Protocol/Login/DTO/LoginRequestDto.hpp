#ifndef HEADER__FILE__LOGINREQUESTDTO
#define HEADER__FILE__LOGINREQUESTDTO

#include <string_view>

struct LoginRequestDto
{
    std::string_view id;
    std::string_view password;
};

#endif /* HEADER__FILE__LOGINREQUESTDTO */
