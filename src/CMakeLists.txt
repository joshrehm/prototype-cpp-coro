#
# Set up our executable
add_executable(prototype_cpp_coro
    "prototype_cpp_coro.cpp"
  "scheduler.cpp")

set_target_properties(prototype_cpp_coro PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED TRUE
)
set_standard_target_options(prototype_cpp_coro)


#
# Set output directory
set_target_properties(prototype_cpp_coro PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PROTOTYPE_CPP_CORO_OUTPUT_PATH})

# Set target includes
target_include_directories(prototype_cpp_coro
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/../include)
