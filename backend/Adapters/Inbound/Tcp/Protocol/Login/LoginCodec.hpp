#ifndef ADAPTERS_INBOUND_TCP_PROTOCOL_LOGIN_LOGINCODEC_HPP
#define ADAPTERS_INBOUND_TCP_PROTOCOL_LOGIN_LOGINCODEC_HPP

#include <cstddef>
#include <optional>
#include <vector>

#include "Domain/LoginResult.hpp"
#include "Protocol/Common/ApiVersion.hpp"
#include "Protocol/Login/DTO/LoginRequestDto.hpp"

namespace LoginCodec
{
std::optional<LoginRequestDto> DecodeRequest(ApiVersion::Type api_version,
                                             const std::vector<std::byte> &payload);
std::optional<std::vector<std::byte>> EncodeResponse(ApiVersion::Type api_version, LoginResult result_code);
} // namespace LoginCodec

#endif /* ADAPTERS_INBOUND_TCP_PROTOCOL_LOGIN_LOGINCODEC_HPP */
