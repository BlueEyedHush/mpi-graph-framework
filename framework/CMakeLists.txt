cmake_minimum_required(VERSION 3.8.1)
project(framework)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS_DEBUG "-ggdb3 -O0 -pedantic -rdynamic") # rdynamic for function names in backtraces
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(CMAKE_CXX_FLAGS_NOLOCAL "-ggdb3 -O3 -DGCM_NO_LOCAL_SHORTCIRCUIT")
set(IT_TEST_PROCESS_COUNT 3)

# for absolute includes
include_directories(src/main)

# sandbox
file(GLOB_RECURSE SANDBOX_SRC src/sandbox/*.cpp src/sandbox/*.h)
# add_executable(sandbox ${SANDBOX_SRC})

# for testing particular compilation units during large refactorings
file(STRINGS "single_cu.filelist" SINGLE_CU)
# add_library(single_cu ${SINGLE_CU})

# executable configuration
file(GLOB_RECURSE FRAMEWORK_SRC src/main/*.cpp src/main/*.h)
add_library(framework_lib ${FRAMEWORK_SRC})

# glog
link_directories(lib/glog/.libs)
include_directories(lib/glog/src)
target_link_libraries(framework_lib glog)

set(FRAMEWORK_MAIN src/entry_point/main.cpp)
add_executable(framework ${FRAMEWORK_MAIN})
target_link_libraries(framework framework_lib)

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
    message(STATUS "Found MPI.\n Includes: ${MPI_INCLUDE_PATH}\n Libs: ${MPI_C_LIBRARIES}")
    include_directories(SYSTEM ${MPI_INCLUDE_PATH})
    target_link_libraries(framework ${MPI_C_LIBRARIES})
else()
    message(FATAL_ERROR "Can't find MPI")
endif()

# tests configuration
set(GTEST_ROOT lib/googletest/googletest)
enable_testing()
find_package(GTest REQUIRED)
if(GTEST_FOUND)
    message(STATUS "Found GoogleTest")
    include_directories(${GTEST_INCLUDE_DIRS})

    if(MPI_C_FOUND)
        file(GLOB_RECURSE TEST_SRC src/test/*.cpp src/test/*.h)
        add_executable(tests ${TEST_SRC})
        target_link_libraries(tests ${GTEST_BOTH_LIBRARIES} ${MPI_C_LIBRARIES} framework_lib)
        add_test(UnitTests tests)

        file(GLOB_RECURSE IT_TEST_SRC src/it/*.cpp src/it/*.h)
        add_executable(itTests ${IT_TEST_SRC})
        target_link_libraries(itTests ${GTEST_LIBRARIES} ${MPI_C_LIBRARIES} framework_lib)
        add_test(NAME ItTests COMMAND mpiexec -np ${IT_TEST_PROCESS_COUNT} -l cmake-build-relwithdebinfo/itTests)
        # ./itTests --gtest_filter=ColouringValidator.AcceptsCorrectSolutionForSTG
    else()
        message(FATAL_ERROR "Can't find MPI, so cannot build integration tests")
    endif()
else()
    message(FATAL_ERROR "Can't find GoogleTest")
endif()
