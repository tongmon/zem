find_package(GTest CONFIG REQUIRED)

file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS ./*.cpp)

add_executable(ZEMTests ${SRC_FILES})

target_link_libraries(ZEMTests PRIVATE
    AdaptersOutboundDb
    Core
    GTest::gmock
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(ZEMTests)
