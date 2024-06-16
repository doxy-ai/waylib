cmake_minimum_required(VERSION 3.20)
project(Waylib LANGUAGES C CXX)

set(GLM_ENABLE_CXX_20 ON CACHE BOOL "Enable C++ 20")
set(GLM_ENABLE_LANG_EXTENSIONS ON CACHE BOOL "Enable language extensions")
add_subdirectory(thirdparty/glm)

add_executable(waylib waylib.cpp)
target_link_libraries(waylib PUBLIC glm::glm)
set_property(TARGET waylib PROPERTY CXX_STANDARD 20)
target_compile_definitions(waylib PUBLIC WAYLIB_NAMESPACE_NAME=wl)