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
if(EMSCRIPTEN)
	add_library(glfw INTERFACE)
	target_link_options(glfw INTERFACE -sUSE_GLFW=3)
endif()
set(GLM_ENABLE_CXX_20 ON CACHE BOOL "Enable C++ 20")
set(GLM_ENABLE_LANG_EXTENSIONS ON CACHE BOOL "Enable language extensions")
add_subdirectory(thirdparty/glm)
add_subdirectory(thirdparty/glfw3webgpu)


add_library(waylib_common common.cpp math.cpp)
target_link_libraries(waylib_common PUBLIC webgpu glm::glm)
target_compile_definitions(waylib_common PUBLIC WAYLIB_NAMESPACE_NAME=wl)
if(EMSCRIPTEN)
	target_link_options(waylib_common PUBLIC
		-sASYNCIFY # Required by WebGPU-C++
	)
endif()
if(MSVC OR EMSCRIPTEN)
	add_library(waylib_common_bundled ALIAS waylib_common)
else()
	bundle_static_library(waylib_common waylib_common_bundled)
endif()


add_library(waylib waylib.cpp shader_preprocessor.cpp)
target_link_libraries(waylib PUBLIC waylib_common_bundled)

add_library(waylib_window window.cpp)
add_library(waylib::window ALIAS waylib_window)
target_link_libraries(waylib_window PUBLIC glfw glfw3webgpu waylib) # NOTE: Waylib window integrates a bit closer than other modules so links to waylib

add_library(waylib_texture texture.cpp thirdparty/miniz.c)
add_library(waylib::texture ALIAS waylib_texture)
target_link_libraries(waylib_texture PUBLIC waylib_common_bundled)

add_library(waylib_model model.cpp)
add_library(waylib::model ALIAS waylib_model)
target_link_libraries(waylib_model PUBLIC waylib_common_bundled)

add_executable(waylib_tests test.cpp)
target_link_libraries(waylib_tests PUBLIC waylib waylib::window waylib::texture waylib::model)
add_custom_command(TARGET waylib_tests POST_BUILD
   COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_SOURCE_DIR}/resources ${CMAKE_CURRENT_BINARY_DIR}/resources DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/resources)

if(EMSCRIPTEN)
	set_target_properties(waylib_tests PROPERTIES SUFFIX ".html")
	target_link_options(waylib_tests PUBLIC
		-fexceptions
		-sALLOW_MEMORY_GROWTH
		--preload-file ${CMAKE_CURRENT_SOURCE_DIR}/resources@resources
	)
endif()
