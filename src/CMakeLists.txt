#
# Set up our executable
add_executable(prototype_cpp_coro
    "prototype_cpp_coro.cpp"
 )

set_standard_target_options(prototype_cpp_coro)

#
# Enable CppCheck
if (PROTOTYPE_CPP_CORO_ENABLE_CPPCHECK)
    include(CppCheck)
    enable_cppcheck(prototype_cpp_coro)
endif()

#
# Set output directory
set_target_properties(prototype_cpp_coro PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PROTOTYPE_CPP_CORO_OUTPUT_PATH})

# Set target includes
target_include_directories(prototype_cpp_coro
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/../include)

#
# Set target libraries
find_package(fmt REQUIRED)
target_link_libraries(prototype_cpp_coro PRIVATE
    fmt::fmt
)
