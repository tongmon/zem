find_package(boost_asio CONFIG REQUIRED)

file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS
    "./Application/*.cpp"
    "./Domain/*.cpp"
)

add_library(Core STATIC ${SRC_FILES})

target_link_libraries(Core PUBLIC
    Boost::asio
)

target_include_directories(Core PUBLIC
    "${CMAKE_SOURCE_DIR}/Core"
)
