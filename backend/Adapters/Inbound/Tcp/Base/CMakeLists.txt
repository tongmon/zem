find_package(boost_asio CONFIG REQUIRED)

add_library(TCPBase INTERFACE)

target_link_libraries(TCPBase INTERFACE
    Boost::asio
    spdlog_header_only)

target_include_directories(TCPBase INTERFACE "./")
