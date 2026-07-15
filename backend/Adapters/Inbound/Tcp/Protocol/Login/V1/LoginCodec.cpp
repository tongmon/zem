#include "Protocol/Login/V1/LoginCodec.hpp"
#include "Protocol/Common/ByteCodec.hpp"
#include "Protocol/WireTypes.hpp"

// Login request body: | Id Length | Id | Pw Length | Pw |
// Login response body: | Result Length | Result Code |

std::optional<V1::LoginRequestDto> V1::LoginCodec::DecodeRequest(const std::vector<std::byte> &payload)
{
    ByteCodec::Reader reader(payload);
    std::string_view id_view;
    std::string_view pw_view;

    if (!reader.ReadLenPrefixed(id_view))
        return std::nullopt;

    if (!reader.ReadLenPrefixed(pw_view))
        return std::nullopt;

    if (!reader.Empty())
        return std::nullopt;

    return LoginRequestDto{id_view, pw_view};
}

std::optional<std::vector<std::byte>> V1::LoginCodec::EncodeResponse(const V1::LoginResponseDto &response)
{
    constexpr TCPWireTypes::DataLengthType result_field_size = sizeof(TCPWireTypes::ServiceCodeType);
    std::vector<std::byte> payload;

    ByteCodec::Writer writer(payload, result_field_size + sizeof(response.result_code));
    writer.Write(response.result_code);

    return payload;
}
