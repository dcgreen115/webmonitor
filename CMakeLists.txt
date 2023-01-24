cmake_minimum_required(VERSION 3.23)
project(pingmonitor)

set(CMAKE_CXX_STANDARD 23)
find_package(CURL REQUIRED)
find_package(cpp-terminal REQUIRED)

add_executable(pingmonitor src/main.cpp src/monitor.hpp src/monitor.cpp src/terminal.hpp src/terminal.cpp)

include_directories(${CURL_INCLUDE_DIR})
include_directories(${CPP-TERMINAL_INCLUDE_DIR})
target_link_libraries(pingmonitor CURL::libcurl)
target_link_libraries(pingmonitor cpp-terminal)
