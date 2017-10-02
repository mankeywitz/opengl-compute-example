cmake_minimum_required(VERSION 3.5)
project(particle)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

set(CMAKE_CXX_STANDARD 14)

add_subdirectory(deps/glfw-3.2.1)

include_directories(deps/glm)
include_directories(deps/glad/include)
include_directories(deps/glfw-3.2.1/include)

set(SRC_FILES main.cpp deps/glad/src/glad.c)

add_executable(particle ${SRC_FILES})

target_link_libraries(particle glfw dl)



