#include "Application/UseCases/LoginUseCase.hpp"
#include "Application/Ports/Outbound/IAccountRepository.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/use_future.hpp>
#include <gtest/gtest.h>

namespace
{
class FakeAccountRepository : public IAccountRepository
{
  public:
    std::optional<std::string> password_to_return;
    std::string requested_id;

    Task<std::optional<std::string>> GetPassword(std::string_view id) override
    {
        requested_id = std::string(id);
        co_return password_to_return;
    }
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

TEST(LoginUseCaseTest, ReturnsSuccessWhenPasswordMatches)
{
    auto repo = std::make_shared<FakeAccountRepository>();
    repo->password_to_return = std::string("pw");

    LoginUseCase use_case(repo);

    auto result = RunTask(use_case.ValidateCredentials("alice", "pw"));

    EXPECT_EQ(result, LoginResult::Success);
    EXPECT_EQ(repo->requested_id, "alice");
}

TEST(LoginUseCaseTest, ReturnsPwInconsistencyWhenPasswordDiffers)
{
    auto repo = std::make_shared<FakeAccountRepository>();
    repo->password_to_return = std::string("pw");

    LoginUseCase use_case(repo);

    auto result = RunTask(use_case.ValidateCredentials("alice", "wrong"));

    EXPECT_EQ(result, LoginResult::InvalidCredentials);
}

TEST(LoginUseCaseTest, ReturnsNonexistentIdWhenRepositoryHasNoPassword)
{
    auto repo = std::make_shared<FakeAccountRepository>();
    repo->password_to_return = std::nullopt;

    LoginUseCase use_case(repo);

    auto result = RunTask(use_case.ValidateCredentials("alice", "pw"));

    EXPECT_EQ(result, LoginResult::InvalidCredentials);
}
