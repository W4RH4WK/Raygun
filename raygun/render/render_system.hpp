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

#pragma once

#include "raygun/assert.hpp"
#include "raygun/camera.hpp"
#include "raygun/compute/compute_system.hpp"
#include "raygun/config.hpp"
#include "raygun/gpu/gpu_buffer.hpp"
#include "raygun/gpu/uniform_buffer.hpp"
#include "raygun/render/fade.hpp"
#include "raygun/render/imgui_renderer.hpp"
#include "raygun/render/raytracer.hpp"
#include "raygun/render/swapchain.hpp"
#include "raygun/scene.hpp"
#include "raygun/vulkan_context.hpp"
#include "raygun/window.hpp"

namespace raygun::render {

/// Main render system which maintains specific renderers and required
/// boilerplate.
class RenderSystem {
  public:
    RenderSystem();
    ~RenderSystem();

    void preSimulation();

    void render(Scene& scene);

    void setupModelBuffers();
    void updateModelBuffers();

    vk::RenderPass& renderPass()
    {
        if(!m_renderPass) {
            RAYGUN_FATAL("RenderPass not set");
        }

        return *m_renderPass;
    }

    Swapchain& swapchain()
    {
        if(!m_swapchain) {
            RAYGUN_FATAL("Swapchain not set");
        }

        return *m_swapchain;
    }

    Raytracer& raytracer()
    {
        if(!m_raytracer) {
            RAYGUN_FATAL("Raytracer not set");
        }

        return *m_raytracer;
    }

    void resetUniformBuffer();

    template<class F, typename... Args>
    void makeFade(Args&&... args)
    {
        if(!m_currentFade || m_currentFade->over()) {
            m_currentFade = std::make_unique<F>(std::forward<Args>(args)...);
        }
    }

  private:
    static constexpr vk::SampleCountFlagBits SAMPLES = vk::SampleCountFlagBits::e1;

    vk::UniqueRenderPass m_renderPass;

    UniqueSwapchain m_swapchain;

    vk::UniqueCommandBuffer m_commandBuffer;
    vk::UniqueFence m_commandBufferFence;

    UniqueRaytracer m_raytracer;

    UniqueImGuiRenderer m_imGuiRenderer;

    gpu::UniqueBuffer m_uniformBuffer;
    gpu::UniqueBuffer m_vertexBuffer;
    gpu::UniqueBuffer m_indexBuffer;
    gpu::UniqueBuffer m_materialBuffer;

    uint32_t m_framebufferIndex = 0;

    vk::UniqueSemaphore m_imageAcquiredSemaphore;
    vk::UniqueSemaphore m_renderCompleteSemaphore;

    VulkanContext& vc;

    std::unique_ptr<Fade> m_currentFade;

    void updateUniformBuffer(const Camera& camera);
    void updateVertexAndIndexBuffer(std::set<Mesh*>& meshes);
    void updateMaterialBuffer(std::vector<Model*>& models);

    void beginFrame();
    void endFrame(std::vector<vk::Semaphore> waitSemaphores = {});

    void beginRenderPass();
    void endRenderPass();

    void presentFrame();

    void resize();

    void setupRenderPass();
};

using UniqueRenderSystem = std::unique_ptr<RenderSystem>;

} // namespace raygun::render
