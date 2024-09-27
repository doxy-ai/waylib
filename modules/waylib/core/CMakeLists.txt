add_subdirectory(thirdparty/WebGPU-distribution)

add_library(waylib_core core.cpp)
target_link_libraries(waylib_core PUBLIC webgpu)

set(MODULE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../../)
cmake_path(ABSOLUTE_PATH MODULE_DIRECTORY NORMALIZE)
target_include_directories(waylib_core PUBLIC ${MODULE_DIRECTORY})

add_library(waylib::core ALIAS waylib_core)

include(CheckCXXSourceCompiles)
check_cxx_source_compiles("#include <expected>
int main() {
	std::expected<int, int> works;
}" STD_EXPECTED_SUPPORTED)
if(NOT STD_EXPECTED_SUPPORTED)
	message("-- Fetching supplemental expected library...")

	FetchContent_Declare(fetch-expected GIT_REPOSITORY https://github.com/TartanLlama/expected.git)
	set(EXPECTED_BUILD_TESTS off CACHE BOOL "Enable tl::expected tests")
	set(EXPECTED_BUILD_PACKAGE_DEB off CACHE BOOL "Create a DEB")
	FetchContent_MakeAvailable(fetch-expected)
	target_link_libraries(waylib_core PUBLIC expected)
endif()