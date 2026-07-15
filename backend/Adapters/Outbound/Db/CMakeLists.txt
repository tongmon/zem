find_package(boost_asio CONFIG REQUIRED)
find_package(SOCI CONFIG REQUIRED)
find_package(PostgreSQL REQUIRED)

file(GLOB SRC_FILES CONFIGURE_DEPENDS ./*.cpp)

add_library(AdaptersOutboundDb STATIC ${SRC_FILES})

target_link_libraries(AdaptersOutboundDb PUBLIC
    Core
    Boost::asio
    SOCI::soci_postgresql_static
    SOCI::soci_core_static
    PostgreSQL::PostgreSQL
    spdlog_header_only
)

target_include_directories(AdaptersOutboundDb PUBLIC
    "./"
)
