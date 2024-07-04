cmake_minimum_required(VERSION 3.20)
project(Waylib LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)

if(CMAKE_BUILD_TYPE MATCHES Debug)
	message("Debug Build... Some Optimizations Disabled")
else()
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

include(cmake/bundle.cmake)

add_subdirectory(thirdparty/WebGPU-distribution)
set(GLM_ENABLE_CXX_20 ON CACHE BOOL "Enable C++ 20")
set(GLM_ENABLE_LANG_EXTENSIONS ON CACHE BOOL "Enable language extensions")
add_subdirectory(thirdparty/glm)
add_subdirectory(thirdparty/glfw3webgpu)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "If the test suite for Assimp is built in addition to the library.")
add_subdirectory(thirdparty/assimp)

add_library(waylib_common common.cpp math.cpp)
target_link_libraries(waylib_common PUBLIC webgpu glm::glm)
target_compile_definitions(waylib_common PUBLIC WAYLIB_NAMESPACE_NAME=wl)
bundle_static_library(waylib_common waylib_common_bundled)

add_library(waylib waylib.cpp shader_preprocessor.cpp)
target_link_libraries(waylib PUBLIC glm::glm waylib_common_bundled)
target_compile_definitions(waylib PUBLIC WAYLIB_NAMESPACE_NAME=wl)

add_library(waylib_window window.cpp)
add_library(waylib::window ALIAS waylib_window)
target_link_libraries(waylib_window PUBLIC glfw glfw3webgpu waylib) # NOTE: Waylib window integrates a bit closer than other modules so links to waylib
target_compile_definitions(waylib_window PUBLIC WAYLIB_NAMESPACE_NAME=wl)

add_library(waylib_texture texture.cpp thirdparty/miniz.c)
add_library(waylib::texture ALIAS waylib_texture)
target_link_libraries(waylib_texture PUBLIC waylib_common_bundled)
target_compile_definitions(waylib_texture PUBLIC WAYLIB_NAMESPACE_NAME=wl)

add_library(waylib_model model.cpp)
add_library(waylib::model ALIAS waylib_model)
target_link_libraries(waylib_model PUBLIC assimp::assimp waylib_texture)
target_compile_definitions(waylib_model PUBLIC WAYLIB_NAMESPACE_NAME=wl)

add_executable(waylib_tests test.cpp)
target_link_libraries(waylib_tests PUBLIC waylib waylib::window waylib::model)
