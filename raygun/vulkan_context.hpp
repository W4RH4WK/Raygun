// The MIT License (MIT)
//
// Copyright (c) 2019-2021 The Raygun Authors.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.

#pragma once

#include "raygun/gpu/gpu_queue.hpp"
#include "raygun/utils/vulkan_type_utils.hpp"
#include "raygun/window.hpp"

namespace raygun {

/// Vulkan boilerplate class. This one holds the most essential resources
/// relevant to all renderers.
class VulkanContext {
  private:
    // Needs to be destroyed last.
    std::shared_ptr<spdlog::logger> m_logger;

  public:
    vk::Extent2D windowSize;

    vk::UniqueInstance instance;

    vk::UniqueDebugUtilsMessengerEXT debugMessenger;

    vk::UniqueSurfaceKHR surface;
    vk::Format surfaceFormat = vk::Format::eUndefined;

    vk::PhysicalDevice physicalDevice;
    vk::PhysicalDeviceProperties physicalDeviceProperties;
    vk::PhysicalDeviceSubgroupProperties physicalDeviceSubgroupProperties;

    vk::UniqueDevice device;

    uint32_t graphicsQueueFamilyIndex = UINT32_MAX;
    uint32_t presentQueueFamilyIndex = UINT32_MAX;
    uint32_t computeQueueFamilyIndex = UINT32_MAX;

    gpu::UniqueQueue graphicsQueue;
    gpu::UniqueQueue presentQueue;
    gpu::UniqueQueue computeQueue;

    //////////////////////////////////////////////////////////////////////////

    void waitIdle();

    void waitForFence(vk::Fence fence);

    template<typename T>
    void setObjectName([[maybe_unused]] const T& object, [[maybe_unused]] string_view name)
    {
        static_assert(vulkan::is_vulkan_type_v<T>);

#ifndef NDEBUG
        vk::DebugMarkerObjectNameInfoEXT info = {};
        info.setObjectType(vulkan::getDebugReportObjectTypeEXT<T>());
        info.setObject(reinterpret_cast<const uint64_t&>(object));
        info.setPObjectName(name.data());

        device->debugMarkerSetObjectNameEXT(info);
#endif
    }

    //////////////////////////////////////////////////////////////////////////

    VulkanContext();
    ~VulkanContext();

  private:
    void setupInstance();

    void setupDebug();

    void setupPhysicalDevice();

    void setupSurface(const Window& window);

    void selectQueueFamily();

    void setupDevice();

    void setupQueues();
};

using UniqueVulkanContext = std::unique_ptr<VulkanContext>;

} // namespace raygun
