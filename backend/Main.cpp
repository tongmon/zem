#include "App/Bootstrap/AppContext.hpp"

#include <boost/system/system_error.hpp>
#include <soci/soci.h>
#include <spdlog/spdlog.h>

int main(int argc, char *argv[])
{
    try
    {
        // MongoDB 연결 정보 초기화
        // MongoDBPool::Get({"localhost", "27017", "Minigram", "tongstar", "@Lsy12131213", 4, 4});

        // MongoDBClient::Get({"localhost", "27017", "Minigram", "tongstar", "@Lsy12131213"});
        // MongoDBClient::Free();

        // init logger
        // auto stdout_logger = spdlog::create_async<spdlog::sinks::stdout_color_sink_mt>("wms_console_logger");

        auto app = AppContext::CreateDefault();
        app->Run();
    }
    catch (boost::system::system_error &e)
    {
        spdlog::error("Error occured! Error: {}", e.what());
    }
    catch (soci::soci_error const &e)
    {
        spdlog::error("DB error occured! Error: {}", e.what());
    }
    catch (std::exception const &e)
    {
        spdlog::error("Unknown error occured! Error: {}", e.what());
    }

    return 0;
}
