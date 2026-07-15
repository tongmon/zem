#include "Protocol/Login/LoginCodec.hpp"

#include "Protocol/Login/V1/LoginCodec.hpp"

std::optional<LoginRequestDto> LoginCodec::DecodeRequest(ApiVersion::Type api_version,
                                                         const std::vector<std::byte> &payload)
{
    switch (api_version)
    {
    case ApiVersion::V1: {
        auto decoded = V1::LoginCodec::DecodeRequest(payload);
        if (!decoded)
            return std::nullopt;

        return LoginRequestDto{decoded->id, decoded->password};
    }
    default:
        return std::nullopt;
    }
}

std::optional<std::vector<std::byte>> LoginCodec::EncodeResponse(ApiVersion::Type api_version, LoginResult result_code)
{
    switch (api_version)
    {
    case ApiVersion::V1:
        return V1::LoginCodec::EncodeResponse(V1::LoginResponseDto{result_code});
    default:
        return std::nullopt;
    }
}
