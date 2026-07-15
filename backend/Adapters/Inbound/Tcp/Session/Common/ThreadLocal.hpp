#ifndef HEADER__FILE__TCPSESSIONTHREADLOCAL
#define HEADER__FILE__TCPSESSIONTHREADLOCAL

#include <random>

namespace TCPSessionThreadLocal
{
inline thread_local std::mt19937 RandomGenerator(std::random_device{}());
}

#endif /* HEADER__FILE__TCPSESSIONTHREADLOCAL */
