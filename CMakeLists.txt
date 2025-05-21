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
