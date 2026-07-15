#ifndef HEADER__FILE__TCPSESSION
#define HEADER__FILE__TCPSESSION

#include "Base/Session.hpp"
#include "Protocol/Common/ApiVersion.hpp"
#include "Protocol/IConnection.hpp"
#include "Protocol/Service/Protocol.hpp"

#include <array>
#include <atomic>
#include <chrono>
#include <deque>
#include <functional>
#include <memory>
#include <vector>

#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/use_awaitable.hpp>

class TCPSession : public Session<TCPSession>, public IConnection
{
  public:
    using Handler = std::function<boost::asio::awaitable<void>(std::shared_ptr<IConnection>,
                                                               ApiVersion::Type,
                                                               std::shared_ptr<std::vector<std::byte>>)>;

  private:
    struct WriteData
    {
        std::array<std::byte, sizeof(TCPHeaderConv)> header{};
        std::shared_ptr<std::vector<std::byte>> body = nullptr;
        std::size_t total_bytes = 0;
    };

    bool m_is_writing;
    std::deque<WriteData> m_write_queue;
    std::size_t m_pending_bytes;

    std::chrono::steady_clock::time_point m_last_rx;
    boost::asio::steady_timer m_hb_timer;
    std::uniform_int_distribution<int> m_hb_jitter;

    boost::asio::steady_timer m_resume_read;
    bool m_backpressure;
    ApiVersion::Type m_peer_api_version;

    std::array<Handler, static_cast<std::size_t>(TCPServiceType::TypeCnt)> m_handlers;

    std::atomic<bool> m_is_closed;

    boost::asio::awaitable<void> ReadLoop();
    void Drain(); // write queue drain

    boost::asio::awaitable<void> Heartbeat();
    void SendPing();
    void SendPong();

  public:
    TCPSession(std::shared_ptr<boost::asio::ip::tcp::socket> sock,
               boost::asio::io_context::executor_type exec,
               std::array<Handler, static_cast<std::size_t>(TCPServiceType::TypeCnt)> handlers);
    ~TCPSession();
    void StartHandling() override;
    void Shutdown() override;
    void SendFrame(TCPServiceType type,
                   std::shared_ptr<std::vector<std::byte>> body = nullptr,
                   ApiVersion::Type api_version = ApiVersion::Latest);
    void Send(TCPServiceType type,
              std::shared_ptr<std::vector<std::byte>> body,
              ApiVersion::Type api_version) override;
    void Close() override;
};

#endif /* HEADER__FILE__TCPSESSION */
