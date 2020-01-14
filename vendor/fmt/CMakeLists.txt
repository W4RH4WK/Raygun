add_library(fmt src/format.cc src/posix.cc)
target_include_directories(fmt PUBLIC include)
set_target_properties(fmt PROPERTIES FOLDER vendor)
