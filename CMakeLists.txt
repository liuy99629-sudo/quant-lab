cmake_minimum_required(VERSION 3.20)
project(quant_lab LANGUAGES CXX)

if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Debug)
endif()

option(QL_ASAN "Build with AddressSanitizer + UBSan" OFF)
option(QL_TSAN "Build with ThreadSanitizer" OFF)

if(QL_ASAN AND QL_TSAN)
  message(FATAL_ERROR "QL_ASAN and QL_TSAN cannot be enabled together")
endif()

if(QL_ASAN)
  add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=undefined)
  add_link_options(-fsanitize=address,undefined)
endif()

if(QL_TSAN)
  add_compile_options(-fsanitize=thread -fno-omit-frame-pointer)
  add_link_options(-fsanitize=thread)
endif()

# GoogleTest and Google Benchmark: use the copies installed on the system if
# there are any (apt: libgtest-dev libbenchmark-dev; brew: googletest google-benchmark),
# otherwise download them at configure time.
include(FetchContent)

find_package(GTest QUIET)
if(NOT GTest_FOUND)
  message(STATUS "GoogleTest not found on the system, downloading it")
  set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_Declare(googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.tar.gz)
  FetchContent_MakeAvailable(googletest)
endif()

find_package(benchmark QUIET)
if(NOT benchmark_FOUND)
  message(STATUS "Google Benchmark not found on the system, downloading it")
  set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
  set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
  set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
  set(BENCHMARK_ENABLE_WERROR OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(benchmark
    URL https://github.com/google/benchmark/archive/refs/tags/v1.9.1.tar.gz)
  FetchContent_MakeAvailable(benchmark)
endif()

add_library(ql_warnings INTERFACE)
target_compile_options(ql_warnings INTERFACE -Wall -Wextra -Wpedantic -Wshadow)

add_library(ql_common INTERFACE)
target_include_directories(ql_common INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/common/include)
target_link_libraries(ql_common INTERFACE ql_warnings)

enable_testing()
include(GoogleTest)

# ql_add_test(<target> <label> <sources...>)
function(ql_add_test name label)
  add_executable(${name} ${ARGN})
  target_link_libraries(${name} PRIVATE ql_common GTest::gtest_main)
  gtest_discover_tests(${name} PROPERTIES LABELS ${label} DISCOVERY_TIMEOUT 60)
endfunction()

add_subdirectory(stage0)
add_subdirectory(stage1/trade_stats)
add_subdirectory(stage1/containers)
