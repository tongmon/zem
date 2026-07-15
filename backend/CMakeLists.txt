cmake_minimum_required(VERSION 4.0.3)

execute_process(
  COMMAND "git clean -fd && git pull"
  WORKING_DIRECTORY ${CMAKE_TOOLCHAIN_FILE}/../../..)
  
execute_process(
  COMMAND vcpkg x-update-baseline --add-initial-baseline
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

add_custom_target(Graphviz ALL
    COMMAND dot -Tpng ${CMAKE_BINARY_DIR}/GraphInfo/graph.dot -o ${CMAKE_BINARY_DIR}/GraphInfo/Generated${CMAKE_BUILD_TYPE}LibGraph.png
)

set(MYPROJECT_VERSION "1.0.0")
project("ZEM_v${MYPROJECT_VERSION}" VERSION ${MYPROJECT_VERSION} LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(BUILD_SHARED_LIBS OFF)
include(CTest)

find_package(boost_filesystem CONFIG REQUIRED)
find_package(boost_dll CONFIG REQUIRED)

add_subdirectory(Library) 
add_subdirectory(Core)
add_subdirectory(Adapters)
add_subdirectory(App/Bootstrap)
if (BUILD_TESTING)
  add_subdirectory(Tests)
endif()

file(GLOB SRC_FILES CONFIGURE_DEPENDS ./*.cpp)

add_executable(${PROJECT_NAME} ${SRC_FILES})

if (WIN32)
  if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set_property(TARGET ${PROJECT_NAME} PROPERTY
      MSVC_RUNTIME_LIBRARY "MultiThreadedDebug"
    )
  elseif (CMAKE_BUILD_TYPE STREQUAL "Release")
    set_property(TARGET ${PROJECT_NAME} PROPERTY
      MSVC_RUNTIME_LIBRARY "MultiThreaded"
    )
  endif()
endif()

target_link_libraries(${PROJECT_NAME} PUBLIC 
    AppBootstrap
)
