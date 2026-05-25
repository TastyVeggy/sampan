cmake_minimum_required(VERSION 3.28)

project(sampan VERSION 0.1.0 DESCRIPTION "Browser engine for the Yuloh language" LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS NO)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(SAMPAN_BUILD_TESTS "Build unit and integration tests" ON)

add_library(sampan_warnings INTERFACE)
target_compile_options(sampan_warnings INTERFACE
    -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion
    -Wold-style-cast -Wcast-align -Wnon-virtual-dtor -Woverloaded-virtual
    -Wnull-dereference -Wdouble-promotion -Wformat=2 -Werror)

add_library(sampan_sanitizers INTERFACE)
target_compile_options(sampan_sanitizers INTERFACE
    $<$<CONFIG:Debug>:-fsanitize=address,undefined -fno-omit-frame-pointer>)
target_link_options(sampan_sanitizers INTERFACE
    $<$<CONFIG:Debug>:-fsanitize=address,undefined>)

if(SAMPAN_BUILD_TESTS)
    enable_testing()
    include(FetchContent)
    set(INSTALL_GTEST OFF CACHE BOOL "Do not install GoogleTest with Sampan" FORCE)
    FetchContent_Declare(googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(googletest)
endif()

add_subdirectory(src)
if(SAMPAN_BUILD_TESTS)
    add_subdirectory(tests)
endif()
