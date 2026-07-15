#include "Handlers/Login/LoginHandler.hpp"

#include <cstdint>
#include <utility>

#include <spdlog/spdlog.h>

#include "Protocol/Login/LoginCodec.hpp"

LoginHandler::LoginHandler(std::shared_ptr<ILoginUseCase> usecase)
    : m_usecase{std::move(usecase)}
{
}

boost::asio::awaitable<void> LoginHandler::operator()(std::shared_ptr<IConnection> connection,
                                                      ApiVersion::Type api_version,
                                                      std::shared_ptr<std::vector<std::byte>> data_buf)
{
    if (!m_usecase)
    {
        spdlog::error("LoginUseCase is not configured.");
        co_return;
    }

    if (!data_buf)
    {
        spdlog::error("Invalid login payload (null).");
        co_return;
    }

    auto request_dto = LoginCodec::DecodeRequest(api_version, *data_buf);
    if (!request_dto)
    {
        spdlog::error("Invalid login payload. Api version: {}", api_version);
        co_return;
    }

    auto result_code = co_await m_usecase->ValidateCredentials(request_dto->id, request_dto->password);
    auto encoded_body = LoginCodec::EncodeResponse(api_version, result_code);
    if (!encoded_body)
    {
        spdlog::error("Failed to encode login response. Api version: {}", api_version);
        co_return;
    }
    auto body = std::make_shared<std::vector<std::byte>>(std::move(*encoded_body));

    if (connection)
        connection->Send(TCPServiceType::Login, body, api_version);
    else
        spdlog::error("Connection is null. Cannot send login response.");

    co_return;
}
