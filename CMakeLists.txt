cmake_minimum_required(VERSION 3.12)
project(osc)

include(FetchContent)
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw
    GIT_TAG 3.3.8
)

set(BUILD_SHARED_LIBS OFF)
set(GLFW_BUILD_EXAMPLES OFF)
set(GLFW_BUILD_TESTS OFF)
set(GLFW_BUILD_DOCS OFF)
set(GLFW_INSTALL OFF)
FetchContent_MakeAvailable(glfw)

find_package(OpenGL REQUIRED)

add_executable(osc main.cpp)

target_include_directories(osc PUBLIC ${glfw_SOURCE_DIR}/include)
target_include_directories(osc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(osc glfw)

target_link_libraries(osc ${OPENGL_gl_LIBRARY})

target_include_directories(osc PUBLIC "${src_dir}/include/")
target_include_directories(osc PUBLIC "${CMAKE_SOURCE_DIR}/include/")

set_target_properties(osc PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set_target_properties(osc PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/bin")
