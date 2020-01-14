file(GLOB_RECURSE example_srcs *.cpp *.hpp)

add_executable(example ${example_srcs})
target_link_libraries(example PRIVATE raygun)

raygun_enable_warnings(example)
raygun_handle_copy_dlls(example)
raygun_set_source_groups(example)

set_target_properties(example PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
