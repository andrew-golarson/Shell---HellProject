cmake_minimum_required(VERSION 3.14)

project(shell-starter-cpp)

set(CMAKE_CXX_STANDARD 17)

file(GLOB_RECURSE SOURCE_FILES src/*.cpp src/*.hpp)

add_executable(shell ${SOURCE_FILES})

target_include_directories(shell PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/)