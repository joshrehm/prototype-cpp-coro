#
# Add our test executable
add_executable(prototype_cpp_coro_tests
    "prototype_cpp_coro_tests.cpp"
)

set_standard_target_options(prototype_cpp_coro_tests)

#
# Enable CppCheck
if (PROTOTYPE_CPP_CORO_ENABLE_CPPCHECK)
    include(CppCheck)
    enable_cppcheck(prototype_cpp_coro_tests)
endif()

#
# Set output directory
set_target_properties(prototype_cpp_coro_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PROTOTYPE_CPP_CORO_OUTPUT_PATH})

# Set target includes
target_include_directories(prototype_cpp_coro_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/../include)

#
# Set target libraries
find_package(Catch2 REQUIRED)
find_package(fmt REQUIRED)

target_link_libraries(prototype_cpp_coro_tests PRIVATE
    Catch2::Catch2WithMain
    fmt::fmt
)

#
# Discover tests and automatically run on build
include(Catch)
catch_discover_tests(prototype_cpp_coro_tests)

if (PROTOTYPE_CPP_CORO_RUN_TESTS_ON_BUILD)
    add_custom_command(TARGET prototype_cpp_coro_tests POST_BUILD
                       COMMAND ctest --output-on-failure
                       WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
endif()
