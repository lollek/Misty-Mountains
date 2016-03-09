cmake_minimum_required(VERSION 3.3)
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
project(misty_mountains)

set (PROJECT_VERSION "v2.0-alpha2-dev")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Weverything -Werror -std=c++11 -Wno-c++98-compat-pedantic -Wno-padded -Wno-c++11-compat")

file(GLOB_RECURSE PROJECT_SOURCES "${PROJECT_SOURCE_DIR}/src/*.cc")
file(GLOB_RECURSE PROJECT_HEADERS "${PROJECT_SOURCE_DIR}/src/*.h")

set (PREFIX "/usr/local")
set (SCOREPATH "\"${PREFIX}/share/${PROJECT_NAME}/highscore\"")

configure_file (
  "${PROJECT_SOURCE_DIR}/src/misty_mountains.h.in"
  "${PROJECT_BINARY_DIR}/include/misty_mountains.h"
  )

find_package(Curses REQUIRED)

include_directories(${PROJECT_BINARY_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/include)

add_executable (${PROJECT_NAME} ${PROJECT_SOURCES})
target_link_libraries(${PROJECT_NAME} Curses)

# Create ctags automatically
add_custom_command(
  OUTPUT tags
  COMMAND ctags -R src/* || true)
add_custom_target(run_ctags DEPENDS tags)
add_dependencies(${PROJECT_NAME} run_ctags)
