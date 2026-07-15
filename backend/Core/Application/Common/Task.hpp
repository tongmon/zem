#ifndef HEADER__FILE__TASK
#define HEADER__FILE__TASK

#include <boost/asio/awaitable.hpp>

template <class T>
using Task = boost::asio::awaitable<T>;

#endif /* HEADER__FILE__TASK */
