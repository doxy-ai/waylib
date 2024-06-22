cmake_minimum_required(VERSION 3.20)
project(Waylib LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(thirdparty/WebGPU-distribution)
set(GLM_ENABLE_CXX_20 ON CACHE BOOL "Enable C++ 20")
set(GLM_ENABLE_LANG_EXTENSIONS ON CACHE BOOL "Enable language extensions")
add_subdirectory(thirdparty/glm)
add_subdirectory(thirdparty/glfw3webgpu)
add_subdirectory(thirdparty/assimp)



add_library(waylib waylib.cpp)
target_link_libraries(waylib PUBLIC webgpu glm::glm)
target_compile_definitions(waylib PUBLIC WAYLIB_NAMESPACE_NAME=wl)

add_library(waylib_window window.cpp)
add_library(waylib::window ALIAS waylib_window)
target_link_libraries(waylib_window PUBLIC glfw glfw3webgpu waylib)
target_compile_definitions(waylib_window PUBLIC WAYLIB_NAMESPACE_NAME=wl)

add_library(waylib_texture texture.cpp thirdparty/miniz.c)
add_library(waylib::texture ALIAS waylib_texture)
target_link_libraries(waylib_texture PUBLIC waylib)
target_compile_definitions(waylib_texture PUBLIC WAYLIB_NAMESPACE_NAME=wl)

add_library(waylib_model model.cpp)
add_library(waylib::model ALIAS waylib_model)
target_link_libraries(waylib_model PUBLIC assimp::assimp waylib_texture)
target_compile_definitions(waylib_model PUBLIC WAYLIB_NAMESPACE_NAME=wl)

add_executable(waylib_tests test.cpp)
target_link_libraries(waylib_tests PUBLIC waylib waylib::window waylib::model)
