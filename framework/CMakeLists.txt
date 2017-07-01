cmake_minimum_required(VERSION 3.8.1)
project(framework)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS_DEBUG "-ggdb3 -O0")

# for absolute includes
include_directories(src/framework)

# executable configuration
file(GLOB_RECURSE FRAMEWORK_SRC src/framework/*.cpp src/framework/*.h)
add_library(framework_lib ${FRAMEWORK_SRC})

set(FRAMEWORK_MAIN src/framework_main/main.cpp)
add_executable(framework ${FRAMEWORK_MAIN})
target_link_libraries(framework framework_lib)

# tests configuration
set(GTEST_ROOT lib/googletest/googletest)
enable_testing()
find_package(GTest REQUIRED)
if(GTEST_FOUND)
    message(STATUS "Found GoogleTest")
    file(GLOB_RECURSE TEST_SRC src/framework_tests/*.cpp src/framework_tests/*.h)
    add_executable(tests ${TEST_SRC})
    include_directories(${GTEST_INCLUDE_DIRS})
    target_link_libraries(tests ${GTEST_BOTH_LIBRARIES} framework_lib)
else()
    message(FATAL_ERROR "Can't find GoogleTest")
endif()

add_test(AllTests tests)

# dependencies
# Boost
set(BOOST_ROOT lib/boost_1_64_0)
set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
set(Boost_NO_SYSTEM_PATHS ON)
find_package(Boost 1.64.0 COMPONENTS program_options REQUIRED)

MESSAGE(STATUS "** Boost Include: ${Boost_INCLUDE_DIR}")
MESSAGE(STATUS "** Boost Libraries: ${Boost_LIBRARIES}")
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    target_link_libraries(framework ${Boost_LIBRARIES})
else()
    message(FATAL_ERROR "Can't find Boost")
endif()

# MPI
find_package(MPI REQUIRED)
if(MPI_C_FOUND)
    message(STATUS "Found MPI")
    include_directories(SYSTEM ${MPI_INCLUDE_PATH})
    target_link_libraries(framework ${MPI_C_LIBRARIES})
else()
    message(FATAL_ERROR "Can't find MPI")
endif()
