cmake_minimum_required(VERSION 3.14)
include(FetchContent)
project(shell-starter-cpp)

set(CMAKE_CXX_STANDARD 17)

file(GLOB_RECURSE SOURCE_FILES src/*.cpp src/*.hpp)

add_executable(shell ${SOURCE_FILES})

FetchContent_Declare(
    replxx
    GIT_REPOSITORY https://github.com/AmokHuginnsson/replxx
    GIT_TAG        master
)

FetchContent_MakeAvailable(replxx)

target_link_libraries(shell PRIVATE replxx::replxx)