#include "PostgresAccountRepository.hpp"

#include <boost/asio/post.hpp>
#include <boost/asio/this_coro.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <optional>
#include <spdlog/spdlog.h>

PostgresAccountRepository::PostgresAccountRepository(std::shared_ptr<soci::connection_pool> pool,
                                                     boost::asio::any_io_executor db_exec)
    : m_pool{std::move(pool)},
      m_db_exec{std::move(db_exec)}
{
}

boost::asio::awaitable<std::optional<std::string>> PostgresAccountRepository::GetPassword(std::string_view id)
{
    auto caller_exec = co_await boost::asio::this_coro::executor;
    co_await boost::asio::post(m_db_exec, boost::asio::use_awaitable);

    std::optional<std::string> password = std::nullopt;
    bool leased = false;
    std::size_t pos = 0;
    constexpr int lease_timeout_ms = 2000;

    try
    {
        if (!m_pool || !m_pool->try_lease(pos, lease_timeout_ms))
        {
            spdlog::error("DB pool lease timeout ({} ms).", lease_timeout_ms);
        }
        else
        {
            leased = true;

            soci::session &sql = m_pool->at(pos);
            soci::indicator pw_ind;
            std::string stored_password;

            sql << "select password from account where id=:id",
                soci::into(stored_password, pw_ind),
                soci::use(std::string(id));

            if (sql.got_data() && pw_ind == soci::i_ok)
                password = stored_password;
        }
    }
    catch (const std::exception &e)
    {
        spdlog::error("DB error on GetPassword: {}", e.what());
    }

    if (leased)
        m_pool->give_back(pos);

    co_await boost::asio::post(caller_exec, boost::asio::use_awaitable);
    co_return password;
}

// Task<LoginResult> PostgresAccountRepository::ValidateCredentials(std::string id, std::string password)
// {
//     auto caller_exec = co_await boost::asio::this_coro::executor;
//     co_await boost::asio::post(m_db_exec, boost::asio::use_awaitable);
//
//     LoginResult result = LoginResult::NonexistentId;
//     bool leased = false;
//     std::size_t pos = 0;
//     constexpr int lease_timeout_ms = 2000;
//
//     try
//     {
//         if (!m_pool || !m_pool->try_lease(pos, lease_timeout_ms))
//         {
//             spdlog::error("DB pool lease timeout ({} ms).", lease_timeout_ms);
//             result = LoginResult::DbError;
//         }
//         else
//         {
//             leased = true;
//
//             soci::session &sql = m_pool->at(pos);
//             soci::indicator pw_ind;
//             std::string stored_password;
//
//             sql << "select password from account where id=:id",
//                 soci::into(stored_password, pw_ind),
//                 soci::use(id);
//
//             if (sql.got_data())
//             {
//                 if (pw_ind == soci::i_ok)
//                     result = (password == stored_password) ? LoginResult::Success : LoginResult::PwInconsistency;
//                 else
//                     result = LoginResult::NonexistentPw;
//             }
//             else
//             {
//                 result = LoginResult::NonexistentId;
//             }
//         }
//     }
//     catch (const std::exception &e)
//     {
//         spdlog::error("DB error on ValidateCredentials: {}", e.what());
//         result = LoginResult::DbError;
//     }
//
//     if (leased)
//         m_pool->give_back(pos);
//
//     co_await boost::asio::post(caller_exec, boost::asio::use_awaitable);
//     co_return result;
// }
