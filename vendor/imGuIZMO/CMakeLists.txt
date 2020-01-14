add_library(imGuIZMO include/imGuIZMO/imGuIZMO.cpp)
target_include_directories(imGuIZMO PUBLIC include)
target_link_libraries(imGuIZMO PUBLIC glm imgui)
set_target_properties(imGuIZMO PROPERTIES FOLDER vendor)
