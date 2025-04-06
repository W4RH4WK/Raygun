file(GLOB_RECURSE big_example_srcs *.cpp *.hpp)

add_executable(big_example ${big_example_srcs})
target_link_libraries(big_example PRIVATE raygun dl)

raygun_enable_warnings(big_example)
raygun_handle_copy_dlls(big_example)
raygun_set_source_groups(big_example)

set_target_properties(big_example PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
