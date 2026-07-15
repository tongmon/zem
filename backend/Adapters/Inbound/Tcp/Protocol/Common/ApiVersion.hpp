#ifndef HEADER__FILE__APIVERSION
#define HEADER__FILE__APIVERSION

#include "Protocol/Service/Protocol.hpp"

namespace ApiVersion
{
using Type = TCPWireTypes::ApiVersionType;

inline constexpr Type V1 = 1;
// inline constexpr Type V2 = 2;

inline constexpr Type Latest = V1;
inline constexpr Type MinSupported = V1;

constexpr bool IsSupported(Type version)
{
    return version >= MinSupported && version <= Latest;
}

constexpr bool IsSupported(TCPServiceType service_type, Type version)
{
    if (!IsSupported(version))
        return false;

    switch (service_type)
    {
    case TCPServiceType::Ping:
    case TCPServiceType::Pong:
    case TCPServiceType::Login:
    case TCPServiceType::Receiving:
        return version == V1;
    default:
        return false;
    }
}
} // namespace ApiVersion

#endif /* HEADER__FILE__APIVERSION */
