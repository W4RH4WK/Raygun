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

#include "raygun/render/imgui_renderer.hpp"

#include "raygun/config.hpp"
#include "raygun/logging.hpp"
#include "raygun/raygun.hpp"
#include "raygun/render/render_system.hpp"

namespace raygun::render {

ImGuiRenderer::ImGuiRenderer(RenderSystem& renderSystem)
    : iniLocation((configDirectory() / "imgui.ini").string())
    , window(RG().window())
    , vc(RG().vc())
    , renderSystem(renderSystem)
{
    ImGui::CreateContext();

    ImGui::GetIO().IniFilename = iniLocation.c_str();

    ImGui::StyleColorsDark();
    ImGui::GetStyle().WindowRounding = 0.0f;
    ImGui::GetStyle().ChildRounding = 0.0f;
    ImGui::GetStyle().FrameRounding = 0.0f;
    ImGui::GetStyle().GrabRounding = 0.0f;
    ImGui::GetStyle().PopupRounding = 0.0f;
    ImGui::GetStyle().ScrollbarRounding = 0.0f;

    ImGui_ImplGlfw_InitForVulkan(window.window(), true);

    setupDescriptorPool();

    setupForVulkan();

    setupFonts();

    RAYGUN_INFO("ImGui initialized");
}

ImGuiRenderer::~ImGuiRenderer()
{
    vc.waitIdle();

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void ImGuiRenderer::newFrame()
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void ImGuiRenderer::render(vk::CommandBuffer& cmd)
{
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
}

void ImGuiRenderer::setupDescriptorPool()
{
    // Taken from ImGui!

    const std::vector<vk::DescriptorPoolSize> sizes = {
        {vk::DescriptorType::eSampler, 1000},
        {vk::DescriptorType::eCombinedImageSampler, 1000},
        {vk::DescriptorType::eSampledImage, 1000},
        {vk::DescriptorType::eStorageImage, 1000},
        {vk::DescriptorType::eUniformTexelBuffer, 1000},
        {vk::DescriptorType::eStorageTexelBuffer, 1000},
        {vk::DescriptorType::eUniformBuffer, 1000},
        {vk::DescriptorType::eStorageBuffer, 1000},
        {vk::DescriptorType::eUniformBufferDynamic, 1000},
        {vk::DescriptorType::eStorageBufferDynamic, 1000},
        {vk::DescriptorType::eInputAttachment, 1000},
    };

    vk::DescriptorPoolCreateInfo info = {};
    info.setMaxSets((uint32_t)(1000 * sizes.size()));
    info.setPoolSizeCount((uint32_t)sizes.size());
    info.setPPoolSizes(sizes.data());
    info.setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet);

    descriptorPool = vc.device->createDescriptorPoolUnique(info);
    vc.setObjectName(*descriptorPool, "ImGui");
}

void ImGuiRenderer::setupForVulkan()
{
    ImGui_ImplVulkan_InitInfo info = {};
    info.Instance = *vc.instance;
    info.PhysicalDevice = vc.physicalDevice;
    info.Device = *vc.device;
    info.QueueFamily = vc.graphicsQueue->familyIndex();
    info.Queue = vc.graphicsQueue->queue();
    // info.PipelineCache
    info.DescriptorPool = *descriptorPool;
    info.MinImageCount = 2;
    info.ImageCount = renderSystem.swapchain().imageCount();
    // info.CheckVkResultFn

    if(!ImGui_ImplVulkan_Init(&info, renderSystem.renderPass())) {
        RAYGUN_FATAL("Could not initialize ImGui");
    }
}

void ImGuiRenderer::setupFonts()
{
    auto cmd = vc.graphicsQueue->createCommandBuffer();
    cmd->begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});

    ImGui_ImplVulkan_CreateFontsTexture(*cmd);

    cmd->end();

    vc.graphicsQueue->submit(*cmd);

    vc.waitIdle();

    ImGui_ImplVulkan_DestroyFontUploadObjects();
}

} // namespace raygun::render
