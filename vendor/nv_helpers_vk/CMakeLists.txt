add_library(nv_helpers_vk
    include/nv_helpers_vk/BottomLevelASGenerator.cpp
    include/nv_helpers_vk/DescriptorSetGenerator.cpp
    include/nv_helpers_vk/RaytracingPipelineGenerator.cpp
    include/nv_helpers_vk/ShaderBindingTableGenerator.cpp
    include/nv_helpers_vk/TopLevelASGenerator.cpp
    include/nv_helpers_vk/VKHelpers.cpp
    include/nv_helpers_vk/vulkan_raytracing.cpp
)
target_include_directories(nv_helpers_vk PUBLIC include)
target_link_libraries(nv_helpers_vk PUBLIC Vulkan::Vulkan glm)
set_target_properties(nv_helpers_vk PROPERTIES FOLDER vendor)
