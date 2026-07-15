#ifndef HEADER__FILE__WIRETYPES
#define HEADER__FILE__WIRETYPES

#include <cstddef>
#include <cstdint>

#include <boost/endian/arithmetic.hpp>

namespace TCPWireTypes
{
using DataLengthType = std::uint32_t;
using ServiceCodeType = std::uint32_t;
using ApiVersionType = std::uint16_t;

constexpr std::size_t DataLengthFieldSize = sizeof(DataLengthType);
constexpr std::size_t ServiceCodeFieldSize = sizeof(ServiceCodeType);
constexpr std::size_t ApiVersionFieldSize = sizeof(ApiVersionType);

// BE = Big Endian
using DataLengthBE =
    boost::endian::endian_arithmetic<boost::endian::order::big, DataLengthType, DataLengthFieldSize * 8>;
using ServiceCodeBE =
    boost::endian::endian_arithmetic<boost::endian::order::big, ServiceCodeType, ServiceCodeFieldSize * 8>;
using ApiVersionBE =
    boost::endian::endian_arithmetic<boost::endian::order::big, ApiVersionType, ApiVersionFieldSize * 8>;
} // namespace TCPWireTypes

#endif /* HEADER__FILE__WIRETYPES */
