cmake_minimum_required(VERSION 3.23)
project(webmonitor)

set(CMAKE_CXX_STANDARD 23)
find_package(CURL REQUIRED)
find_package(cpp-terminal REQUIRED)

add_executable(webmonitor src/main.cpp src/monitor.hpp src/monitor.cpp src/terminal.hpp src/terminal.cpp)

include_directories(${CURL_INCLUDE_DIR})
include_directories(${CPP-TERMINAL_INCLUDE_DIR})
target_link_libraries(webmonitor CURL::libcurl)
target_link_libraries(webmonitor cpp-terminal)

target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
