file(GLOB SRC_FILES CONFIGURE_DEPENDS ./*.cpp)

add_library(TCPServer STATIC ${SRC_FILES})

if (WIN32)
  if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set_property(TARGET TCPServer PROPERTY
      MSVC_RUNTIME_LIBRARY "MultiThreadedDebug"
    )
  elseif (CMAKE_BUILD_TYPE STREQUAL "Release")
    set_property(TARGET TCPServer PROPERTY
      MSVC_RUNTIME_LIBRARY "MultiThreaded"
    )
  endif()
endif()

target_link_libraries(TCPServer PUBLIC 
    TCPSession
    AdaptersInboundTcp
    Core
)

target_include_directories(TCPServer PUBLIC
    "${CMAKE_CURRENT_LIST_DIR}/.."
)
