add_subdirectory(Base)
add_subdirectory(Session)

file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS 
    "${CMAKE_CURRENT_LIST_DIR}/Protocol/*.cpp" 
    "${CMAKE_CURRENT_LIST_DIR}/Handlers/*.cpp"
)

add_library(AdaptersInboundTcp STATIC ${SRC_FILES})

target_link_libraries(AdaptersInboundTcp PUBLIC
    Core
    TCPSession
)

target_include_directories(AdaptersInboundTcp PUBLIC
    "${CMAKE_CURRENT_LIST_DIR}"
)

add_subdirectory(Server)
