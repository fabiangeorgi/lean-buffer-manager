cmake_minimum_required(VERSION 3.16)
project(buffer_manager)
set(CMAKE_CXX_STANDARD 20)

include(FetchContent)

option(CI_BUILD "Set to ON for complete build in CI." OFF)

if(NOT CMAKE_BUILD_TYPE)
    message(STATUS "No build type specified. Defaulting to Debug.
                    Use cmake `-DCMAKE_BUILD_TYPE=Release ..` to build an optimized version.")
endif()

if (CMAKE_BUILD_TYPE MATCHES Release)
    message(STATUS "Building an optimized release version.")
    add_compile_options(-O3)
    if (CI_BUILD)
        # We cannot set this generally, as it does not work on all architectures.
        message(STATUS "Adding -march/tune=native in CI")
        add_compile_options(-march=native -mtune=native)
    endif()
endif()

set(TASK_SOURCES
        src/buffer_frame.cpp
        src/buffer_frame.hpp
        src/buffer_manager.cpp
        src/buffer_manager.hpp
        src/data_regions.cpp
        src/data_regions.hpp
        src/swip.cpp
        src/swip.hpp
)

add_library(buffer_manager ${TASK_SOURCES})
target_include_directories(buffer_manager INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/src
                                          PUBLIC ${PMDK_INCLUDE_DIRS})

enable_testing()
FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.13.0
)

set(BUILD_GMOCK ON CACHE BOOL "" FORCE)
set(BUILD_GTEST ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

set(TEST_UTILS test/test_utils.cpp test/test_utils.hpp)

add_executable(basic_test test/basic.cpp ${TEST_UTILS})
add_test(basic_test basic_test)
target_link_libraries(basic_test buffer_manager gtest gmock)

if (${CI_BUILD})
    # Build advanced tests in CI only
    add_executable(advanced_test test/advanced.cpp ${TEST_UTILS})
    add_test(advanced_test advanced_test)
    target_link_libraries(advanced_test buffer_manager gtest gmock)

    add_executable(hdp_benchmark test/benchmark.cpp)
    target_link_libraries(hdp_benchmark buffer_manager)
endif()
