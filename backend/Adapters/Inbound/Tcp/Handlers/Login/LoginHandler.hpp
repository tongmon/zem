#ifndef HEADER__FILE__LOGINHANDLER
#define HEADER__FILE__LOGINHANDLER

#include <cstddef>
#include <memory>
#include <vector>

#include <boost/asio/awaitable.hpp>

#include "Application/Ports/Inbound/ILoginUseCase.hpp"
#include "Protocol/IConnection.hpp"

class LoginHandler
{
  public:
    explicit LoginHandler(std::shared_ptr<ILoginUseCase> usecase);

    boost::asio::awaitable<void> operator()(std::shared_ptr<IConnection> connection,
                                            ApiVersion::Type api_version,
                                            std::shared_ptr<std::vector<std::byte>> data_buf);

  private:
    std::shared_ptr<ILoginUseCase> m_usecase;
};

#endif /* HEADER__FILE__LOGINHANDLER */
