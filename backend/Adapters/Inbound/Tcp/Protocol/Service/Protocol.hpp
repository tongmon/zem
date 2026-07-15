#ifndef HEADER__FILE__SERVICE_PROTOCOL
#define HEADER__FILE__SERVICE_PROTOCOL

#include "Protocol/WireTypes.hpp"

// 4 bytes for service type, 4 bytes for payload length, 2 bytes for api version
enum class TCPServiceType : TCPWireTypes::ServiceCodeType
{
    Ping = 0,
    Pong,
    Login,
    Receiving,
    TypeCnt,
};
static_assert(sizeof(TCPServiceType) == TCPWireTypes::ServiceCodeFieldSize);

struct alignas(2) TCPHeaderConv
{
    TCPWireTypes::ServiceCodeBE service_type = 0;
    TCPWireTypes::DataLengthBE data_size = 0;
    TCPWireTypes::ApiVersionBE api_version = 0;
};
static_assert(sizeof(TCPHeaderConv) ==
                  TCPWireTypes::ServiceCodeFieldSize + TCPWireTypes::DataLengthFieldSize + TCPWireTypes::ApiVersionFieldSize,
              "TCPHeaderConv must be service_type(4) + data_size(4) + api_version(2)");

#endif /* HEADER__FILE__SERVICE_PROTOCOL */
