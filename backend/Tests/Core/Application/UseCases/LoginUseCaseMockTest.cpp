#include "Application/Ports/Outbound/IAccountRepository.hpp"
#include "Application/UseCases/LoginUseCase.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/use_future.hpp>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace
{
class MockAccountRepository : public IAccountRepository
{
  public:
    MOCK_METHOD((Task<std::optional<std::string>>), GetPassword, (std::string_view id), (override));
};

template <typename T>
T RunTask(Task<T> task)
{
    boost::asio::io_context io;
    auto future = boost::asio::co_spawn(io, std::move(task), boost::asio::use_future);
    io.run();
    return future.get();
}
} // namespace

TEST(LoginUseCaseMockTest, CallsRepositoryWithUserIdAndReturnsSuccess)
{
    auto repo = std::make_shared<testing::StrictMock<MockAccountRepository>>();

    EXPECT_CALL(*repo, GetPassword(std::string_view{"alice"}))
        .WillOnce([](std::string_view) -> Task<std::optional<std::string>> {
            co_return std::optional<std::string>{"pw"};
        });

    LoginUseCase use_case(repo);

    auto result = RunTask(use_case.ValidateCredentials("alice", "pw"));

    EXPECT_EQ(result, LoginResult::Success);
}

TEST(LoginUseCaseMockTest, ReturnsNonexistentIdWhenRepositoryReturnsNullopt)
{
    auto repo = std::make_shared<testing::StrictMock<MockAccountRepository>>();

    EXPECT_CALL(*repo, GetPassword(testing::_))
        .WillOnce([](std::string_view) -> Task<std::optional<std::string>> {
            co_return std::nullopt;
        });

    LoginUseCase use_case(repo);

    auto result = RunTask(use_case.ValidateCredentials("unknown", "pw"));

    EXPECT_EQ(result, LoginResult::InvalidCredentials);
}
