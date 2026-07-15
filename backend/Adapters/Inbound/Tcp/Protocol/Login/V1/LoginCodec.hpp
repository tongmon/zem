#ifndef HEADER__FILE__LOGINCODEC
#define HEADER__FILE__LOGINCODEC

#include <cstddef>
#include <optional>
#include <vector>

#include "Protocol/Login/V1/DTO/LoginRequestDto.hpp"
#include "Protocol/Login/V1/DTO/LoginResponseDto.hpp"

namespace V1::LoginCodec
{
std::optional<LoginRequestDto> DecodeRequest(const std::vector<std::byte> &payload);
std::optional<std::vector<std::byte>> EncodeResponse(const LoginResponseDto &response);
} // namespace V1::LoginCodec

#endif /* HEADER__FILE__LOGINCODEC */
