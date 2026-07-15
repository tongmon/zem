#include "TCPServer.hpp"

#include "Handlers/Login/LoginHandler.hpp"

TCPServer::TCPServer(std::shared_ptr<ILoginUseCase> login_usecase,
                     const unsigned short &port_num,
                     unsigned int thread_pool_size)
    : Server(port_num, thread_pool_size),
      m_login_usecase{std::move(login_usecase)}
{
}

std::unique_ptr<TCPSession> TCPServer::MakeSession(std::shared_ptr<boost::asio::ip::tcp::socket> sock)
{
    std::array<TCPSession::Handler, static_cast<size_t>(TCPServiceType::TypeCnt)> handlers;
    handlers.fill([](std::shared_ptr<IConnection> c, auto, auto) -> Task<void> {
        spdlog::warn("Unhandled service type");
        if (c)
            c->Close();
        co_return;
    });

    handlers[static_cast<size_t>(TCPServiceType::Login)] = LoginHandler{m_login_usecase};

    return std::make_unique<TCPSession>(std::move(sock),
                                        m_ios.get_executor(),
                                        std::move(handlers));
}

TCPServer::~TCPServer()
{
}
