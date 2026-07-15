#include "TCPSession.hpp"

#ifdef _WIN32
#include <Mstcpip.h>
#elif __linux__
#include <netinet/tcp.h>
#include <sys/socket.h>
#endif

#include <format>
#include <fstream>
#include <iostream>
#include <limits>

#include <boost/asio/bind_cancellation_slot.hpp>
#include <boost/asio/cancellation_signal.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/endian/arithmetic.hpp>
#include <boost/endian/conversion.hpp>
#include <spdlog/spdlog.h>

#include "Common/Limits.hpp"
#include "Common/ThreadLocal.hpp"
#include "Common/Timeouts.hpp"

TCPSession::TCPSession(std::shared_ptr<boost::asio::ip::tcp::socket> sock,
                       boost::asio::io_context::executor_type exec,
                       std::array<Handler, static_cast<std::size_t>(TCPServiceType::TypeCnt)> handlers)
    : Session(std::move(sock), exec),
      m_is_writing(false),
      m_pending_bytes(0),
      m_hb_timer(m_strand),
      m_hb_jitter(0, 500), // 0 to 500 ms jitter
      m_resume_read(m_strand),
      m_backpressure(false),
      m_peer_api_version(ApiVersion::Latest),
      m_handlers(std::move(handlers)),
      m_is_closed(false)
{
    boost::system::error_code ec;
    m_sock->set_option(boost::asio::ip::tcp::no_delay(true), ec);
    m_sock->set_option(boost::asio::socket_base::keep_alive(true), ec);

#ifdef _WIN32
    SOCKET s = m_sock->native_handle();
    tcp_keepalive ka{1, 30000, 10000};
    DWORD bytes = 0;
    WSAIoctl(s, SIO_KEEPALIVE_VALS, &ka, sizeof(ka), nullptr, 0, &bytes, nullptr, nullptr);
#elif __linux__
    int fd = m_sock->native_handle();
    int yes = 1, idle = 30, intvl = 10, cnt = 3;
    ::setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &yes, sizeof(yes));
    ::setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &idle, sizeof(idle));
    ::setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &intvl, sizeof(intvl));
    ::setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, &cnt, sizeof(cnt));
#endif
}

TCPSession::~TCPSession()
{
}

void TCPSession::StartHandling()
{
    auto self = shared_from_this();
    m_last_rx = std::chrono::steady_clock::now();

    boost::asio::co_spawn(m_strand, [self]() -> boost::asio::awaitable<void> {
        try
        {
            co_await self->ReadLoop();
        }
        catch (const std::exception &e)
        {
            spdlog::error("ReadLoop crashed: {}", e.what());
            self->Shutdown();
        }
        co_return; }, boost::asio::detached);

    boost::asio::co_spawn(m_strand, [self]() -> boost::asio::awaitable<void> {
        try
        {
            co_await self->Heartbeat();
        }
        catch (const std::exception &e)
        {
            spdlog::error("Heartbeat crashed: {}", e.what());
            self->Shutdown();
        }
        co_return; }, boost::asio::detached);
}

void TCPSession::SendFrame(TCPServiceType type,
                           std::shared_ptr<std::vector<std::byte>> body,
                           ApiVersion::Type api_version)
{
    boost::asio::post(m_strand, [self = shared_from_this(), type, body = std::move(body), api_version]() mutable {
        if (self->m_is_closed.load(std::memory_order_relaxed))
            return;

        WriteData item;
        const auto body_size = body ? body->size() : 0;

        // header serialization
        TCPHeaderConv hdr{static_cast<TCPWireTypes::ServiceCodeType>(type),
                          static_cast<TCPWireTypes::DataLengthType>(body_size),
                          static_cast<TCPWireTypes::ApiVersionType>(api_version)};
        std::memcpy(item.header.data(), &hdr, sizeof(hdr));
        item.total_bytes = sizeof(hdr);

        if (body_size != 0)
        {
            item.body = std::move(body);
            item.total_bytes += body_size;
        }

        // overflow check
        if (body_size > TCPSessionLimit::MaxBodySize ||
            std::numeric_limits<std::size_t>::max() - self->m_pending_bytes < item.total_bytes)
        {
            self->Shutdown();
            return;
        }
        self->m_pending_bytes += item.total_bytes;

        if (self->m_pending_bytes > TCPSessionLimit::HardCap)
        {
            self->Shutdown();
            return;
        }

        // high water mark check
        if (!self->m_backpressure && self->m_pending_bytes > TCPSessionLimit::HighWater)
            self->m_backpressure = true;

        self->m_write_queue.push_back(std::move(item));
        if (!self->m_is_writing)
        {
            self->m_is_writing = true;
            self->Drain();
        }
    });
}

void TCPSession::Send(TCPServiceType type,
                      std::shared_ptr<std::vector<std::byte>> body,
                      ApiVersion::Type api_version)
{
    SendFrame(type, std::move(body), api_version);
}

void TCPSession::Close()
{
    Shutdown();
}

void TCPSession::Drain()
{
    if (m_is_closed.load(std::memory_order_relaxed) || m_write_queue.empty())
    {
        m_is_writing = false;
        return;
    }

    std::array<boost::asio::const_buffer, TCPSessionLimit::MaxIovPerWrite> bufs{};

    std::size_t batch_items = 0,
                batch_bytes = 0,
                used_iov = 0;

    // gather items
    for (auto it = m_write_queue.begin(); it != m_write_queue.end(); ++it)
    {
        const bool has_body = it->body && !it->body->empty();
        const std::size_t seq_size = has_body ? 2 : 1;

        // at least one item should be sended
        // iov/byte limitation check
        if (batch_items && ((used_iov + seq_size > TCPSessionLimit::MaxIovPerWrite) ||
                            (batch_bytes + it->total_bytes > TCPSessionLimit::MaxBatchBytes)))
            break;

        bufs[used_iov++] = boost::asio::buffer(it->header);
        if (has_body)
            bufs[used_iov++] = boost::asio::buffer(*it->body);

        batch_bytes += it->total_bytes;
        ++batch_items;
    }

    boost::asio::async_write(*m_sock, bufs,
                             boost::asio::bind_executor(m_strand,
                                                        [self = shared_from_this(), batch_items, batch_bytes](boost::system::error_code ec, std::size_t) {
                                                            if (ec)
                                                            {
                                                                self->m_is_writing = false;
                                                                self->Shutdown();
                                                                return;
                                                            }

                                                            for (std::size_t i = 0; i < batch_items; ++i)
                                                                self->m_write_queue.pop_front();

                                                            self->m_pending_bytes -= batch_bytes;

                                                            if (self->m_backpressure && self->m_pending_bytes <= TCPSessionLimit::LowWater)
                                                            {
                                                                self->m_backpressure = false;
                                                                self->m_resume_read.cancel();
                                                            }

                                                            self->Drain();
                                                        }));
}

boost::asio::awaitable<void> TCPSession::ReadLoop()
{
    TCPHeaderConv header_conv;
    boost::system::error_code ec;

    while (true)
    {
        if (m_backpressure)
        {
            m_resume_read.expires_at(std::chrono::steady_clock::time_point::max());
            boost::system::error_code ec_wait;
            co_await m_resume_read.async_wait(boost::asio::redirect_error(boost::asio::use_awaitable, ec_wait));
        }

        co_await boost::asio::async_read(*m_sock,
                                         boost::asio::buffer(&header_conv, sizeof(header_conv)),
                                         boost::asio::redirect_error(boost::asio::use_awaitable, ec));
        if (ec)
        {
            spdlog::error("ReadLoop fail on reading header, Error: {}", ec.what());
            Shutdown();
            co_return;
        }

        m_last_rx = std::chrono::steady_clock::now();

        auto service_type =
            static_cast<TCPServiceType>(static_cast<TCPWireTypes::ServiceCodeType>(header_conv.service_type));
        auto data_size = static_cast<TCPWireTypes::DataLengthType>(header_conv.data_size);
        auto api_version = static_cast<ApiVersion::Type>(static_cast<TCPWireTypes::ApiVersionType>(header_conv.api_version));

        spdlog::debug("Header received, Service type: {} / Data size: {} / Api version: {}",
                      static_cast<TCPWireTypes::ServiceCodeType>(service_type),
                      data_size,
                      api_version);

        if (service_type >= TCPServiceType::TypeCnt)
        {
            spdlog::error("Unknown service type, Service Type: {}",
                          static_cast<TCPWireTypes::ServiceCodeType>(service_type));
            Shutdown();
            co_return;
        }

        if (!ApiVersion::IsSupported(service_type, api_version))
        {
            spdlog::error("Unsupported api version for service type, Service type: {} / Api version: {}",
                          static_cast<TCPWireTypes::ServiceCodeType>(service_type),
                          api_version);
            Shutdown();
            co_return;
        }

        m_peer_api_version = api_version;

        if (service_type == TCPServiceType::Ping || service_type == TCPServiceType::Pong)
        {
            if (data_size != 0)
            {
                spdlog::error("Data size should be zero on the ping/pong, Data size: {}", data_size);
                Shutdown();
                co_return;
            }
            if (service_type == TCPServiceType::Ping)
                SendPong();
            continue;
        }
        else if (!data_size || data_size > TCPSessionLimit::MaxBodySize)
        {
            spdlog::error("Invalid data size, Data size: {}", data_size);
            Shutdown();
            co_return;
        }

        auto data_buf = std::make_shared<std::vector<std::byte>>(data_size);
        co_await boost::asio::async_read(*m_sock,
                                         boost::asio::buffer(*data_buf),
                                         boost::asio::redirect_error(boost::asio::use_awaitable, ec));

        if (ec)
        {
            spdlog::error("ReadLoop fail on reading body, Error: {}", ec.what());
            Shutdown();
            co_return;
        }

        m_last_rx = std::chrono::steady_clock::now();

        try
        {
            auto conn = std::static_pointer_cast<IConnection>(shared_from_this());
            co_await m_handlers[static_cast<std::size_t>(service_type)](conn, api_version, data_buf);
        }
        catch (std::exception const &e)
        {
            spdlog::error("ReadLoop fail on executing handler, Error: {}", e.what());
            Shutdown();
            co_return;
        }
    }

    co_return;
}

boost::asio::awaitable<void> TCPSession::Heartbeat()
{
    while (true)
    {
        if (m_is_closed.load(std::memory_order_relaxed))
        {
            spdlog::info("Heartbeat terminated");
            break;
        }

        m_hb_timer.expires_after(TCPSessionTimeout::Interval + std::chrono::milliseconds(m_hb_jitter(TCPSessionThreadLocal::RandomGenerator)));
        boost::system::error_code ec;
        co_await m_hb_timer.async_wait(boost::asio::redirect_error(boost::asio::use_awaitable, ec));
        if (ec == boost::asio::error::operation_aborted)
            continue;

        auto idle = std::chrono::steady_clock::now() - m_last_rx;
        if (idle >= TCPSessionTimeout::HardIdle)
        {
            spdlog::info("Idle time exceeds hard idle limit, Heartbeat terminated, Idle: {}", std::chrono::duration_cast<std::chrono::seconds>(idle));
            Shutdown();
            co_return;
        }

        // wait little longer
        if (idle >= TCPSessionTimeout::SoftIdle)
        {
            // send ping to check the client still alive
            SendPing();

            // graceful wait
            m_hb_timer.expires_after(TCPSessionTimeout::GraceWait);
            co_await m_hb_timer.async_wait(boost::asio::redirect_error(boost::asio::use_awaitable, ec));
            if (ec == boost::asio::error::operation_aborted)
                continue;

            auto idle_after_ping = std::chrono::steady_clock::now() - m_last_rx;
            if (idle_after_ping >= TCPSessionTimeout::HardIdle)
            {
                spdlog::info("Idle time after exceeds hard idle limit, Heartbeat terminated, Idle: {}", std::chrono::duration_cast<std::chrono::seconds>(idle_after_ping));
                Shutdown();
                co_return;
            }
        }
    }

    co_return;
}

void TCPSession::SendPing()
{
    spdlog::debug("Send ping signal!");
    SendFrame(TCPServiceType::Ping, nullptr, m_peer_api_version);
}

void TCPSession::SendPong()
{
    spdlog::debug("Send pong signal!");
    SendFrame(TCPServiceType::Pong, nullptr, m_peer_api_version);
}

void TCPSession::Shutdown()
{
    if (m_is_closed.exchange(true))
        return;

    spdlog::info("Session shutdown!");

    boost::asio::post(m_strand, [self = shared_from_this()] {
        self->m_hb_timer.cancel();
        self->m_resume_read.cancel();

        boost::system::error_code ec;
        self->m_sock->cancel(ec);
        self->m_sock->shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
        self->m_sock->close(ec);
    });
}
