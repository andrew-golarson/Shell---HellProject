cmake_minimum_required(VERSION 3.14)

project(shell-starter-cpp)

set(CMAKE_CXX_STANDARD 17)

include(FetchContent)
FetchContent_Declare(
    linenoise
    GIT_REPOSITORY https://github.com/yhirose/cpp-linenoise.git
    GIT_TAG        master
)
FetchContent_MakeAvailable(linenoise)

file(GLOB_RECURSE SOURCE_FILES src/*.cpp src/*.hpp)

add_executable(shell ${SOURCE_FILES})

target_link_libraries(shell PRIVATE linenoise)

target_include_directories(shell PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)