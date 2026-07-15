#ifndef HEADER__FILE__TCP_SESSIONLIMITS
#define HEADER__FILE__TCP_SESSIONLIMITS

#include <cstddef>

#include "Protocol/WireTypes.hpp"

namespace TCPSessionLimit
{
constexpr TCPWireTypes::DataLengthType MaxBodySize = 2 * 1024 * 1024; // 2MB
constexpr TCPWireTypes::DataLengthType LowWater = 1024 * 512;         // 512KB
constexpr TCPWireTypes::DataLengthType HighWater = LowWater * 2;      // 1MB
constexpr std::size_t HardCap = MaxBodySize * 4;                      // 8MB
constexpr std::size_t MaxBatchBytes = 256 * 1024;                     // 256KB
constexpr std::size_t MaxIovPerWrite = 32;
} // namespace TCPSessionLimit

#endif /* HEADER__FILE__TCP_SESSIONLIMITS */
