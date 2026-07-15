#ifndef HEADER__FILE__LOGINREQUESTDTOV1
#define HEADER__FILE__LOGINREQUESTDTOV1

#include <string_view>

namespace V1
{
struct LoginRequestDto
{
    std::string_view id;
    std::string_view password;
};
} // namespace V1

#endif /* HEADER__FILE__LOGINREQUESTDTOV1 */
