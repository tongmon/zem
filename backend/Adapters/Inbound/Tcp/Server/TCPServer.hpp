#ifndef HEADER__FILE__TCPSERVER
#define HEADER__FILE__TCPSERVER

#include "Application/Ports/Inbound/ILoginUseCase.hpp"
#include "Base/Server.hpp"
#include "Session/TCPSession.hpp"

class TCPServer : public Server<TCPSession>
{
    std::shared_ptr<ILoginUseCase> m_login_usecase;

    std::unique_ptr<TCPSession> MakeSession(std::shared_ptr<boost::asio::ip::tcp::socket> sock) override;

  public:
    TCPServer(std::shared_ptr<ILoginUseCase> login_usecase,
              const unsigned short &port_num,
              unsigned int thread_pool_size = 0);
    ~TCPServer();
};

#endif /* HEADER__FILE__TCPSERVER */
