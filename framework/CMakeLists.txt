cmake_minimum_required(VERSION 3.8.1)
project(framework)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS_DEBUG "-ggdb3 -O0")

# for absolute includes
include_directories(src/)

# executable configuration
file(GLOB_RECURSE SOURCE_FILES src/*.cpp src/*.h)
add_executable(framework ${SOURCE_FILES})


# dependencies
set(BOOST_ROOT lib/boost_1_64_0)
set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
find_package(Boost 1.64.0)

if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    target_link_libraries(framework ${Boost_LIBRARIES})
else()
    message(FATAL_ERROR "Can't find Boost")
endif()

find_package(MPI REQUIRED)
if(MPI_C_FOUND)
    include_directories(SYSTEM ${MPI_INCLUDE_PATH})
    target_link_libraries(framework ${MPI_C_LIBRARIES})
else()
    message(FATAL_ERROR "Can't find MPI")
endif()

