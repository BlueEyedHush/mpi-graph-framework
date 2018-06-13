cmake_minimum_required(VERSION 3.8)
project(er_gen)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS_RELEASE -O3)

set(SOURCE_FILES
        src/er_gen.cpp)

add_executable(er_gen ${SOURCE_FILES})