add_library(imgui
    include/ImGui/imgui.cpp
    include/ImGui/imgui_demo.cpp
    include/ImGui/imgui_draw.cpp
    include/ImGui/imgui_impl_glfw.cpp
    include/ImGui/imgui_impl_vulkan.cpp
    include/ImGui/imgui_tables.cpp
    include/ImGui/imgui_widgets.cpp
    include/ImGui/misc/cpp/imgui_stdlib.cpp
    include/ImGui/misc/fonts/binary_to_compressed_c.cpp
)
target_include_directories(imgui PUBLIC include include/ImGui)
target_link_libraries(imgui PUBLIC Vulkan::Vulkan glfw)
set_target_properties(imgui PROPERTIES FOLDER vendor)
