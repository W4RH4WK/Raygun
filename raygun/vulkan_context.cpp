// The MIT License (MIT)
//
// Copyright (c) 2019,2020 The Raygun Authors.
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

#include "raygun/vulkan_context.hpp"

#include "raygun/assert.hpp"
#include "raygun/info.hpp"
#include "raygun/logging.hpp"
#include "raygun/raygun.hpp"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace raygun {

void VulkanContext::waitIdle()
{
    device->waitIdle();
}

void VulkanContext::waitForFence(vk::Fence fence)
{
    while(true) {
        const auto result = device->waitForFences({fence}, true, 100);
        if(result == vk::Result::eSuccess) break;
        RAYGUN_ASSERT(result == vk::Result::eTimeout);
    }
}

VulkanContext::VulkanContext()
{
    windowSize = RG().windowSize();

    setupInstance();

#ifndef NDEBUG
    setupDebug();
#endif

    setupPhysicalDevice();

    if(!RG().config().headless) {
        setupSurface(RG().window());
    }

    selectQueueFamily();

    setupDevice();

    setupQueues();

    RAYGUN_INFO("Vulkan context initialized");
}

VulkanContext::~VulkanContext()
{
    device->waitIdle();
}

void VulkanContext::setupInstance()
{
    const std::vector<const char*> layers = {
#ifndef NDEBUG
        "VK_LAYER_KHRONOS_validation",
#endif
        "VK_LAYER_LUNARG_monitor",
    };

    std::vector<const char*> extensions = {
#ifndef NDEBUG
        VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif
        VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
    };

    if(!RG().config().headless) {
        const auto glfwExtensions = RG().glfwRuntime().vulkanExtensions();
        extensions.insert(extensions.end(), glfwExtensions.begin(), glfwExtensions.end());
    }

    vk::ApplicationInfo appInfo = {};
    appInfo.setPApplicationName(APP_NAME);
    appInfo.setApplicationVersion(VK_MAKE_VERSION(APP_VERSION_MAJOR, APP_VERSION_MINOR, APP_VERSION_PATCH));
    appInfo.setPEngineName(RAYGUN_NAME);
    appInfo.setEngineVersion(VK_MAKE_VERSION(RAYGUN_VERSION_MAJOR, RAYGUN_VERSION_MINOR, RAYGUN_VERSION_PATCH));

    vk::InstanceCreateInfo info = {};
    info.setEnabledLayerCount((uint32_t)layers.size());
    info.setPpEnabledLayerNames(layers.data());
    info.setEnabledExtensionCount((uint32_t)extensions.size());
    info.setPpEnabledExtensionNames(extensions.data());
    info.setPApplicationInfo(&appInfo);

    vk::DynamicLoader dynamicLoader;
    auto vkGetInstanceProcAddr = dynamicLoader.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
    VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

    instance = vk::createInstanceUnique(info);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(*instance);
}

void VulkanContext::setupDebug()
{
    const auto cb = [](VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT, const VkDebugUtilsMessengerCallbackDataEXT* data,
                       void* userData) -> VkBool32 {
        auto vulkanLogger = static_cast<spdlog::logger*>(userData);

        switch(severity) {
        case VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            vulkanLogger->trace("{}", data->pMessage);
            break;
        case VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            vulkanLogger->info("{}", data->pMessage);
            break;
        case VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            vulkanLogger->warn("{}", data->pMessage);
            break;
        case VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            vulkanLogger->error("{}", data->pMessage);
            break;
        case VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT:
            RAYGUN_FATAL("unreachable");
            break;
        }

        return false;
    };

    m_logger = logger->clone("Vulkan");

    auto severity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
    // severity |= vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo;
    // severity |= vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose;

    const auto type = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation;
    // type |= vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;

    vk::DebugUtilsMessengerCreateInfoEXT info = {};
    info.setMessageSeverity(severity);
    info.setMessageType(type);
    info.setPfnUserCallback(cb);
    info.setPUserData(m_logger.get());

    debugMessenger = instance->createDebugUtilsMessengerEXTUnique(info);
}

void VulkanContext::setupPhysicalDevice()
{
    physicalDevice = instance->enumeratePhysicalDevices().at(0);

    physicalDeviceProperties = physicalDevice.getProperties();

    vk::PhysicalDeviceProperties2 props2;
    props2.pNext = &physicalDeviceSubgroupProperties;
    physicalDevice.getProperties2(&props2);
    RAYGUN_DEBUG("Vulkan physical device subgroup size: {}, arithmetic supported? {}", physicalDeviceSubgroupProperties.subgroupSize,
                 (bool)(physicalDeviceSubgroupProperties.supportedOperations & vk::SubgroupFeatureFlagBits::eArithmetic));
}

void VulkanContext::setupSurface(const Window& window)
{
    surface = window.createSurface(*instance);

    surfaceFormat = physicalDevice.getSurfaceFormatsKHR(*surface).at(0).format;
}

void VulkanContext::selectQueueFamily()
{
    auto families = physicalDevice.getQueueFamilyProperties();

    for(size_t i = 0; i < families.size(); ++i) {
        const auto& family = families[i];

        if(graphicsQueueFamilyIndex == UINT32_MAX && family.queueFlags & vk::QueueFlagBits::eGraphics) {
            graphicsQueueFamilyIndex = i;
        }

        if(RG().config().headless) {
            // Don't really care.
            presentQueueFamilyIndex = graphicsQueueFamilyIndex;
        }
        else if(presentQueueFamilyIndex == UINT32_MAX && physicalDevice.getSurfaceSupportKHR(i, *surface)) {
            presentQueueFamilyIndex = i;
        }

        if(computeQueueFamilyIndex == UINT32_MAX || computeQueueFamilyIndex == graphicsQueueFamilyIndex) {
            if(family.queueFlags & vk::QueueFlagBits::eCompute) {
                computeQueueFamilyIndex = i;
            }
        }
    }

    RAYGUN_ASSERT(graphicsQueueFamilyIndex != UINT32_MAX);
    RAYGUN_ASSERT(presentQueueFamilyIndex != UINT32_MAX);
    RAYGUN_ASSERT(computeQueueFamilyIndex != UINT32_MAX);
}

void VulkanContext::setupDevice()
{
    std::vector<const char*> extensions = {
#ifndef NDEBUG
        VK_EXT_DEBUG_MARKER_EXTENSION_NAME,
#endif
        VK_NV_RAY_TRACING_EXTENSION_NAME,
        VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,
    };

    if(!RG().config().headless) {
        extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    }

    const float queuePriorities[] = {1.0f};

    std::vector<vk::DeviceQueueCreateInfo> queueInfos(2);
    queueInfos[0].setQueueCount(1);
    queueInfos[0].setQueueFamilyIndex(graphicsQueueFamilyIndex);
    queueInfos[0].setPQueuePriorities(queuePriorities);
    queueInfos[1].setQueueCount(1);
    queueInfos[1].setQueueFamilyIndex(computeQueueFamilyIndex);
    queueInfos[1].setPQueuePriorities(queuePriorities);

    if(graphicsQueueFamilyIndex != presentQueueFamilyIndex) {
        vk::DeviceQueueCreateInfo presentQueueInfo = {};
        presentQueueInfo.setQueueCount(1);
        presentQueueInfo.setQueueFamilyIndex(presentQueueFamilyIndex);
        presentQueueInfo.setPQueuePriorities(queuePriorities);

        queueInfos.push_back(presentQueueInfo);
    }

    vk::PhysicalDeviceFeatures feat;
    feat.setRobustBufferAccess(true);

    vk::DeviceCreateInfo info;
    info.setQueueCreateInfoCount((uint32_t)queueInfos.size());
    info.setPQueueCreateInfos(queueInfos.data());
    info.setEnabledExtensionCount((uint32_t)extensions.size());
    info.setPpEnabledExtensionNames(extensions.data());
    info.setPEnabledFeatures(&feat);

    device = physicalDevice.createDeviceUnique(info);
}

void VulkanContext::setupQueues()
{
    graphicsQueue = std::make_unique<gpu::Queue>(*device, graphicsQueueFamilyIndex);

    presentQueue = std::make_unique<gpu::Queue>(*device, presentQueueFamilyIndex);

    computeQueue = std::make_unique<gpu::Queue>(*device, computeQueueFamilyIndex);
}

} // namespace raygun
