#ifndef HEADER__FILE__TCPSESSIONTIMEOUT
#define HEADER__FILE__TCPSESSIONTIMEOUT

#include <chrono>

namespace TCPSessionTimeout
{
constexpr std::chrono::seconds Interval{30};  // heartbeat check interval
constexpr std::chrono::seconds SoftIdle{45};  // ping send threshold
constexpr std::chrono::seconds HardIdle{75};  // shutdown threshold
constexpr std::chrono::seconds GraceWait{10}; // grace period after ping send
} // namespace TCPSessionTimeout

#endif /* HEADER__FILE__TCPSESSIONTIMEOUT */
