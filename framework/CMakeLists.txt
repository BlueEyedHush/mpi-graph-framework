cmake_minimum_required(VERSION 3.6)
project(framework)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS_DEBUG "-ggdb3 -O0")

find_package(MPI REQUIRED)
include_directories(SYSTEM ${MPI_INCLUDE_PATH})

# for absolute includes
include_directories(src/)

file(GLOB_RECURSE SOURCE_FILES src/*.cpp src/*.h)
add_executable(framework ${SOURCE_FILES})
target_link_libraries(framework ${MPI_C_LIBRARIES})