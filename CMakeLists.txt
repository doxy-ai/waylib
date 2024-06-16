cmake_minimum_required(VERSION 3.20)
project(Waylib LANGUAGES C CXX)

set(CXX_STANDARD 20)

set(GLM_ENABLE_CXX_20 ON CACHE BOOL "Enable C++ 20")
set(GLM_ENABLE_LANG_EXTENSIONS ON CACHE BOOL "Enable language extensions")
add_subdirectory(thirdparty/glm)
add_subdirectory(thirdparty/glfw)

add_library(waylib waylib.cpp)
target_link_libraries(waylib PUBLIC glm::glm)
target_compile_definitions(waylib PUBLIC WAYLIB_NAMESPACE_NAME=wl)

add_library(waylib_window window.cpp)
add_library(waylib::window ALIAS waylib_window)
target_link_libraries(waylib_window PUBLIC glfw)
target_compile_definitions(waylib_window PUBLIC WAYLIB_NAMESPACE_NAME=wl)

add_executable(waylib_tests test.cpp)
target_link_libraries(waylib_tests PUBLIC waylib waylib::window)
