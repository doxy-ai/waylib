cmake_minimum_required(VERSION 3.12)
project(waylib CXX C)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(modules/waylib/core)

add_executable(waylib_tests test.cpp)
target_link_libraries(waylib_tests PUBLIC waylib::core)