find_package(boost_dll CONFIG REQUIRED)

file(GLOB SRC_FILES CONFIGURE_DEPENDS ./*.cpp)

add_library(AppBootstrap STATIC ${SRC_FILES})

target_link_libraries(AppBootstrap PUBLIC
    TCPServer
    AdaptersOutboundDb
    Core
    Boost::dll
    spdlog_header_only
)

target_include_directories(AppBootstrap PUBLIC
    "${CMAKE_CURRENT_LIST_DIR}/../.."
)
