#ifndef HEADER__FILE__ICONNECTION
#define HEADER__FILE__ICONNECTION

#include <cstddef>
#include <memory>
#include <vector>

#include "Protocol/Common/ApiVersion.hpp"
#include "Protocol/Service/Protocol.hpp"

class IConnection
{
  public:
    virtual ~IConnection() = default;
    virtual void Send(TCPServiceType type,
                      std::shared_ptr<std::vector<std::byte>> body,
                      ApiVersion::Type api_version) = 0;
    virtual void Close() = 0;
};

#endif /* HEADER__FILE__ICONNECTION */
