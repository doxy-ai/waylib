cmake_minimum_required(VERSION 3.12)
project(stylizer CXX C)

set(CMAKE_CXX_STANDARD 20)

if(CMAKE_BUILD_TYPE MATCHES Debug)
	message("Debug Build... Some Optimizations Disabled")
else()
	set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -sSTACK_OVERFLOW_CHECK=2") #emscripten version

add_subdirectory(modules/stylizer/core)
add_subdirectory(modules/stylizer/img)
add_subdirectory(modules/stylizer/obj)
add_subdirectory(modules/stylizer/window)

add_subdirectory(modules/stylizer/pbr)

add_executable(stylizer_tests test.cpp)
target_link_libraries(stylizer_tests PUBLIC stylizer::core stylizer::img stylizer::obj stylizer::window stylizer::pbr)

if(EMSCRIPTEN)
	set_target_properties(stylizer_tests PROPERTIES SUFFIX ".html")
	target_link_options(stylizer_tests PUBLIC
		# -fexceptions
		# -sALLOW_MEMORY_GROWTH
		--preload-file ${CMAKE_CURRENT_SOURCE_DIR}/resources@resources
	)
endif()