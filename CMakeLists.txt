cmake_minimum_required(VERSION 3.25)

#
# Configure CMake properties
list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

#
# Includes
include(PrototypeCppCoroOptions)

#
# Project setup
generate_version_header("${CMAKE_CURRENT_SOURCE_DIR}/VERSION"
                        "${CMAKE_CURRENT_SOURCE_DIR}/include/prototype_cpp_coro/prototype_cpp_coro_version.h"
                        FROM_TEMPLATE  "prototype_cpp_coro_version.h.in"
                        VARIABLE        PROTOTYPE_CPP_CORO_VERSION)
project(prototype_cpp_coro VERSION ${PROTOTYPE_CPP_CORO_VERSION} LANGUAGES C CXX )

#
# Source files
add_subdirectory(src)

#
# Unit Testing
if (PROTOTYPE_CPP_CORO_RUN_TESTS_ON_BUILD)
    set(PROTOTYPE_CPP_CORO_ENABLE_TESTS ON)
endif()

message_bool(PROTOTYPE_CPP_CORO_ENABLE_TESTS
    "-- Prototype CPP Coro: Testing is enabled"
    "-- Prototype CPP Coro: Testing is disabled")
message_bool(PROTOTYPE_CPP_CORO_RUN_TESTS_ON_BUILD
    "-- Prototype CPP Coro: Tests will run on build"
    "-- Prototype CPP Coro: Tests will not run on build")

if (PROTOTYPE_CPP_CORO_ENABLE_TESTS)
	enable_testing()
	add_subdirectory(tests)
endif()
