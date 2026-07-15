#ifndef HEADER__FILE__BASE_TCP_SESSION
#define HEADER__FILE__BASE_TCP_SESSION

#include <functional>
#include <memory>

#include <boost/asio.hpp>

template <typename Derived>
class Session : public std::enable_shared_from_this<Derived>
{
  protected:
    std::shared_ptr<boost::asio::ip::tcp::socket> m_sock;
    boost::asio::strand<boost::asio::io_context::executor_type> m_strand;

  public:
    Session(std::shared_ptr<boost::asio::ip::tcp::socket> sock, boost::asio::io_context::executor_type exec)
        : m_sock{std::move(sock)}, m_strand{exec}
    {
    }

    virtual ~Session()
    {
    }

    // Child implementations must override this method with 'auto self = shared_from_this();' this code on the first line
    virtual void StartHandling() = 0;

    virtual void Shutdown()
    {
        boost::system::error_code ec;
        m_sock->cancel(ec);
        m_sock->shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
        m_sock->close(ec);
    }
};

#endif /* HEADER__FILE__BASE_TCP_SESSION */
