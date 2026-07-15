#ifndef HEADER__FILE__BASE_TCP_SERVER
#define HEADER__FILE__BASE_TCP_SERVER

#include <atomic>
#include <chrono>
#include <format>
#include <memory>
#include <set>
#include <thread>
#include <vector>

#include <boost/asio.hpp>
#include <spdlog/spdlog.h>

template <typename Session>
class Server : public std::enable_shared_from_this<Server<Session>>
{
  protected:
    boost::asio::io_context m_ios;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> m_work_guard;
    unsigned int m_thread_pool_size;
    std::vector<std::thread> m_thread_pool;
    std::set<std::weak_ptr<Session>, std::owner_less<std::weak_ptr<Session>>> m_sessions;
    boost::asio::strand<boost::asio::io_context::executor_type> m_strand;
    std::unique_ptr<boost::asio::ip::tcp::acceptor> m_acceptor;
    std::atomic<bool> m_is_stopped;

    virtual std::unique_ptr<Session> MakeSession(std::shared_ptr<boost::asio::ip::tcp::socket> sock) = 0;

    void StartAcceptor()
    {
        if (m_is_stopped.load(std::memory_order_relaxed))
            return;

        auto sock = std::make_shared<boost::asio::ip::tcp::socket>(m_ios);
        m_acceptor->async_accept(*sock,
                                 boost::asio::bind_executor(m_strand,
                                                            [this, sock](const boost::system::error_code &ec) {
                                                                try
                                                                {
                                                                    OnAccept(sock, ec);
                                                                }
                                                                catch (const std::exception &e)
                                                                {
                                                                    spdlog::error("Accept fail, Error: {}", e.what());
                                                                }
                                                            }));
    }

    void OnAccept(std::shared_ptr<boost::asio::ip::tcp::socket> sock, const boost::system::error_code &ec)
    {
        if (m_is_stopped.load(std::memory_order_relaxed))
        {
            boost::system::error_code ignored;
            m_acceptor->close(ignored);
            return;
        }

        if (!ec)
        {
            spdlog::info("Accept success! Address: {} / Port: {}", sock->remote_endpoint().address().to_string(), sock->remote_endpoint().port());

            auto uptr = MakeSession(sock);
            auto wp_holder = std::make_shared<std::weak_ptr<Session>>();
            auto deleter = [server_wp = this->weak_from_this(), wp_holder](Session *p) {
                if (auto srv = server_wp.lock())
                {
                    boost::asio::post(srv->m_strand, [srv, wp_holder, p] {
                        srv->m_sessions.erase(*wp_holder);
                        delete p;
                    });
                }
                else
                    delete p;
            };

            std::shared_ptr<Session> session_ptr(uptr.release(), std::move(deleter));
            *wp_holder = session_ptr;
            m_sessions.emplace(*wp_holder);

            session_ptr->StartHandling();

            // Start to accept another session
            StartAcceptor();
        }
        else
        {
            spdlog::error("Accept failed, Try again after 1 sec, Error: {}", ec.what());

            auto timer = std::make_shared<boost::asio::steady_timer>(m_ios, std::chrono::seconds(1));
            timer->async_wait(
                boost::asio::bind_executor(m_strand,
                                           [this, timer](const boost::system::error_code &e) {
                                               StartAcceptor();
                                           }));
        }
    }

  public:
    Server(const unsigned short &port_num, unsigned int thread_pool_size = 0)
        : m_work_guard{boost::asio::make_work_guard(m_ios)},
          m_thread_pool_size{thread_pool_size},
          m_strand{m_ios.get_executor()},
          m_acceptor{std::make_unique<boost::asio::ip::tcp::acceptor>(m_ios, boost::asio::ip::tcp::endpoint(boost::asio::ip::address_v4::any(), port_num))},
          m_is_stopped{false}
    {
        m_acceptor->set_option(boost::asio::socket_base::reuse_address(true));

        if (!thread_pool_size)
            m_thread_pool_size = std::thread::hardware_concurrency() ? std::thread::hardware_concurrency() : 1;

        auto signals = std::make_shared<boost::asio::signal_set>(m_ios, SIGINT, SIGTERM);
        signals->async_wait(boost::asio::bind_executor(m_strand,
                                                       [this, signals](const boost::system::error_code &, int) {
                                                           Stop();
                                                       }));
    }

    virtual ~Server()
    {
        Stop();
    }

    void Run()
    {
        m_acceptor->listen();

        spdlog::info("Server started!, Address: {}, Port: {}, Thread num: {}", m_acceptor->local_endpoint().address().to_string(), m_acceptor->local_endpoint().port(), m_thread_pool_size);

        boost::asio::post(m_strand, [this]() { StartAcceptor(); });

        for (unsigned int i = 0; i < m_thread_pool_size; ++i)
        {
            m_thread_pool.emplace_back([this]() {
                while (true)
                {
                    try
                    {
                        m_ios.run();
                        break;
                    }
                    catch (const std::exception &e)
                    {
                        spdlog::error("Exception occurs on running ios, Error: {}", e.what());
                    }
                }
            });
        }

        for (auto &th : m_thread_pool)
        {
            if (th.joinable())
                th.join();
        }
    }

    void Stop()
    {
        if (m_is_stopped.exchange(true))
            return;

        boost::asio::post(m_strand, [this]() {
            try
            {
                boost::system::error_code ignored;
                m_acceptor->close(ignored);
                for (auto &wp : m_sessions)
                    if (auto session = wp.lock())
                        session->Shutdown();
                m_sessions.clear();
                m_work_guard.reset();
                spdlog::info("Server closed!");
            }
            catch (const std::exception &e)
            {
                spdlog::error("Exception occurs on closing the server, Error: {}", e.what());
            }
        });
    }

    // Copy is not allowed
    Server(const Server &) = delete;
    Server &operator=(const Server &) = delete;
};

#endif /* HEADER__FILE__BASE_TCP_SERVER */
