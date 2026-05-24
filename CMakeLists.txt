cmake_minimum_required(VERSION 3.28)
project(sampan LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_EXTENSIONS NO)

option(SAMPAN_BUILD_TESTS "Build unit testing suite" ON)

add_library(warnings INTERFACE)
target_compile_options(warnings INTERFACE
    -Wall
    -Wextra
    -Wpedantic
    -Wshadow
    -Wconversion
    -Werror
)

add_library(sanitizers INTERFACE)
target_compile_options(sanitizers INTERFACE
    $<$<CONFIG:Debug>:-fsanitize=address,undefined>
)
target_link_options(sanitizers INTERFACE
    $<$<CONFIG:Debug>:-fsanitize=address,undefined>
)

if(SAMPAN_BUILD_TESTS)
    enable_testing()
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.14.0
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif()

add_subdirectory(src)

if(SAMPAN_BUILD_TESTS)
    add_subdirectory(tests)
endif()
