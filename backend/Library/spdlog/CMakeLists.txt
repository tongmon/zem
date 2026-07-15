add_library(spdlog_header_only INTERFACE)

target_include_directories(spdlog_header_only  INTERFACE "./")

target_compile_definitions(spdlog_header_only INTERFACE
    SPDLOG_FMT_EXTERNAL=0
    SPDLOG_USE_STD_FORMAT=1
)