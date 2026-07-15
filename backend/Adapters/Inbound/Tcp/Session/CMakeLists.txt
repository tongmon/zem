file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS ./*.cpp)

add_library(TCPSession STATIC ${SRC_FILES})

if (WIN32)
  if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set_property(TARGET TCPSession PROPERTY
      MSVC_RUNTIME_LIBRARY "MultiThreadedDebug"
    )
  elseif (CMAKE_BUILD_TYPE STREQUAL "Release")
    set_property(TARGET TCPSession PROPERTY
      MSVC_RUNTIME_LIBRARY "MultiThreaded"
    )
  endif()
endif()

target_link_libraries(TCPSession PUBLIC 
    TCPBase
)

target_include_directories(TCPSession PUBLIC
    "${CMAKE_CURRENT_LIST_DIR}/.."
)
