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

#include "raygun/render/render_system.hpp"

#include "raygun/gpu/gpu_material.hpp"
#include "raygun/gpu/gpu_utils.hpp"
#include "raygun/logging.hpp"
#include "raygun/profiler.hpp"
#include "raygun/raygun.hpp"

namespace raygun::render {

RenderSystem::RenderSystem() : vc(RG().vc())
{
    setupRenderPass();

    m_uniformBuffer = gpu::createUniformBuffer();
    resetUniformBuffer();

    m_swapchain = std::make_unique<Swapchain>(*this);

    m_commandBuffer = vc.graphicsQueue->createCommandBuffer();
    vc.setObjectName(*m_commandBuffer, "Render System");

    m_commandBufferFence = vc.device->createFenceUnique({vk::FenceCreateFlagBits::eSignaled});
    vc.setObjectName(*m_commandBufferFence, "Render System");

    m_raytracer = std::make_unique<Raytracer>();

    m_imGuiRenderer = std::make_unique<ImGuiRenderer>(*this);

    m_imageAcquiredSemaphore = vc.device->createSemaphoreUnique({});
    vc.setObjectName(*m_imageAcquiredSemaphore, "Render System Image Acquired");

    m_renderCompleteSemaphore = vc.device->createSemaphoreUnique({});
    vc.setObjectName(*m_renderCompleteSemaphore, "Render System Render Complete");

    RAYGUN_INFO("Render system initialized");
}

RenderSystem::~RenderSystem()
{
    vc.waitIdle();
}

void RenderSystem::reload()
{
    vc.waitIdle();

    vc.windowSize = RG().window().size();

    RG().scene().camera->updateProjection();

    m_swapchain.reset();
    m_swapchain = std::make_unique<Swapchain>(*this);

    m_raytracer.reset();
    m_raytracer = std::make_unique<Raytracer>();

    RAYGUN_INFO("Render System reloaded");
}

void RenderSystem::preSimulation()
{
    m_imGuiRenderer->newFrame();
}

void RenderSystem::render(Scene& scene)
{
    beginFrame();
    {
        RG().profiler().resetVulkanQueries(*m_commandBuffer);

        updateUniformBuffer(*scene.camera);

        m_raytracer->setupTopLevelAS(*m_commandBuffer, scene);

        m_raytracer->updateRenderTarget(*m_uniformBuffer, *m_vertexBuffer, *m_indexBuffer, *m_materialBuffer);

        const auto& raytracerResultImage = m_raytracer->doRaytracing(*m_commandBuffer);

        // Ensure ray traced image is ready for transfer.
        {
            vk::ImageMemoryBarrier barrier;
            barrier.setImage(raytracerResultImage);
            barrier.setOldLayout(vk::ImageLayout::eGeneral);
            barrier.setNewLayout(vk::ImageLayout::eTransferSrcOptimal);
            barrier.setSrcAccessMask(vk::AccessFlagBits::eShaderWrite);
            barrier.setDstAccessMask(vk::AccessFlagBits::eTransferRead);
            barrier.setSubresourceRange(gpu::defaultImageSubresourceRange());

            m_commandBuffer->pipelineBarrier(vk::PipelineStageFlagBits::eRayTracingShaderNV | vk::PipelineStageFlagBits::eComputeShader,
                                             vk::PipelineStageFlagBits::eTransfer, vk::DependencyFlagBits::eByRegion, {}, {}, barrier);
        }

        auto& resultImage = m_swapchain->image(m_framebufferIndex);

        // Transition result image layout for blit.
        {
            vk::ImageMemoryBarrier barr;
            barr.setImage(resultImage);
            barr.setDstAccessMask(vk::AccessFlagBits::eTransferWrite);
            barr.setNewLayout(vk::ImageLayout::eTransferDstOptimal);
            barr.setSubresourceRange(gpu::defaultImageSubresourceRange());

            m_commandBuffer->pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer, //
                                             vk::DependencyFlagBits::eByRegion, {}, {}, barr);
        }

        // Copy ray traced image -> result image.
        {
            vk::Offset3D offset = {0, 0, 0};
            vk::Offset3D bound = {(int32_t)vc.windowSize.width, (int32_t)vc.windowSize.height, 1};

            vk::ImageBlit blit;
            blit.setDstOffsets({offset, bound});
            blit.setDstSubresource(gpu::defaultImageSubresourceLayers());
            blit.setSrcOffsets({offset, bound});
            blit.setSrcSubresource(gpu::defaultImageSubresourceLayers());

            m_commandBuffer->blitImage(raytracerResultImage, vk::ImageLayout::eTransferSrcOptimal, //
                                       resultImage, vk::ImageLayout::eTransferDstOptimal,          //
                                       blit, vk::Filter::eNearest);
        }

        beginRenderPass();
        {
            RG().profiler().doUI();
            gpu::materialEditor();

            m_imGuiRenderer->render(*m_commandBuffer);
        }
        endRenderPass();
    }
    endFrame();

    RG().profiler().endFrame();

    presentFrame();

    vc.waitIdle();
}

namespace {
    struct ModelCounts {
        uint32_t vertexCount = 0;
        uint32_t indexCount = 0;
        uint32_t materialCount = 0;
    };

    ModelCounts getCounts(const std::vector<Model*>& models, const std::set<Mesh*>& meshes)
    {
        ModelCounts ret;
        for(const auto& model: models) {
            ret.materialCount += (uint32_t)model->materials.size();
        }
        for(const auto& mesh: meshes) {
            ret.vertexCount += (uint32_t)mesh->vertices.size();
            ret.indexCount += (uint32_t)mesh->indices.size();
        }
        return ret;
    }

    std::set<Mesh*> distinctMeshes(const std::vector<Model*>& models)
    {
        std::set<Mesh*> result;
        std::transform(models.begin(), models.end(), std::inserter(result, result.end()), [](auto model) { return model->mesh.get(); });
        return result;
    }
} // namespace

void RenderSystem::setupModelBuffers()
{
    auto models = RG().resourceManager().models();
    auto meshes = distinctMeshes(models);

    RAYGUN_INFO("Setting up Model buffers: {} models", models.size());

    auto [vertexCount, indexCount, materialCount] = getCounts(models, meshes);

    m_vertexBuffer = std::make_unique<gpu::Buffer>(vertexCount * sizeof(Vertex),
                                                   vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eStorageBuffer
                                                       | vk::BufferUsageFlagBits::eShaderDeviceAddress,
                                                   vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
    m_vertexBuffer->setName("Vertex Buffer");

    m_indexBuffer = std::make_unique<gpu::Buffer>(indexCount * sizeof(uint32_t),
                                                  vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eStorageBuffer
                                                      | vk::BufferUsageFlagBits::eShaderDeviceAddress,
                                                  vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
    m_indexBuffer->setName("Index Buffer");

    m_materialBuffer = std::make_unique<gpu::Buffer>(materialCount * sizeof(gpu::Material),
                                                     vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress,
                                                     vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
    m_materialBuffer->setName("Material Buffer");

    updateVertexAndIndexBuffer(meshes);

    updateMaterialBuffer(models);
}

void RenderSystem::updateModelBuffers()
{
    auto models = RG().resourceManager().models();
    auto meshes = distinctMeshes(models);

    updateVertexAndIndexBuffer(meshes);

    updateMaterialBuffer(models);
}

void RenderSystem::resetUniformBuffer()
{
    auto& ubo = *static_cast<gpu::UniformBufferObject*>(m_uniformBuffer->map());

    memset(&ubo, 0, sizeof(gpu::UniformBufferObject));

    ubo.lightDir = glm::normalize(vec3(.4f, -.6f, -.8f));
    ubo.numSamples = 1;
    ubo.maxRecursions = 5;
}

void RenderSystem::updateUniformBuffer(const Camera& camera)
{
    auto& ubo = *static_cast<gpu::UniformBufferObject*>(m_uniformBuffer->map());
    ubo.viewInverse = camera.viewInverse();
    ubo.projInverse = camera.projInverse();
    ubo.clearColor = vec3{0.2f, 0.2f, 0.2f};

    // Loop shader time to keep float quality up while also smoothly animating
    // everything which uses trigonometry animation.
    ubo.time = (float)(fmod(RG().time(), (32.0 * glm::pi<double>())));

    // Fade
    if(m_currentFade) {
        ubo.fadeColor = m_currentFade->curColor();
    }

    // UI
    ImGui::SliderInt("SSAA samples", &ubo.numSamples, 1, 32);
    ImGui::SliderInt("Max recursions", &ubo.maxRecursions, 0, 7);
    auto lightLabel = fmt::format("Light Dir {} ###lightdir", ubo.lightDir);
    ImGui::gizmo3D(lightLabel.c_str(), ubo.lightDir);
    ImGui::Checkbox("Show Alpha", &ubo.showAlpha);
}

void RenderSystem::updateVertexAndIndexBuffer(std::set<Mesh*>& meshes)
{
    const auto vertexStart = static_cast<uint8_t*>(m_vertexBuffer->map());
    vk::DeviceSize vertexOffset = 0;

    const auto indexStart = static_cast<uint8_t*>(m_indexBuffer->map());
    vk::DeviceSize indexOffset = 0;

    for(const auto& mesh: meshes) {
        const auto& vertices = mesh->vertices;
        const auto vertexSize = vertices.size() * sizeof(vertices[0]);

        const auto& indices = mesh->indices;
        const auto indexSize = indices.size() * sizeof(indices[0]);

        mesh->vertexBufferRef.bufferAddress = m_vertexBuffer->address();
        mesh->vertexBufferRef.offsetInBytes = vertexOffset;
        mesh->vertexBufferRef.sizeInBytes = vertexSize;
        mesh->vertexBufferRef.elementSize = sizeof(vertices[0]);

        mesh->indexBufferRef.bufferAddress = m_indexBuffer->address();
        mesh->indexBufferRef.offsetInBytes = indexOffset;
        mesh->indexBufferRef.sizeInBytes = indexSize;
        mesh->indexBufferRef.elementSize = sizeof(indices[0]);

        memcpy(vertexStart + vertexOffset, vertices.data(), vertexSize);
        memcpy(indexStart + indexOffset, indices.data(), indexSize);

        vertexOffset += vertexSize;
        indexOffset += indexSize;
    }

    m_vertexBuffer->unmap();
    m_indexBuffer->unmap();
}

void RenderSystem::updateMaterialBuffer(std::vector<Model*>& models)
{
    const auto materialStart = static_cast<uint8_t*>(m_materialBuffer->map());
    vk::DeviceSize materialOffset = 0;

    for(const auto& model: models) {
        const auto& materials = model->materials;
        const auto materialsSize = materials.size() * sizeof(gpu::Material);

        model->materialBufferRef.bufferAddress = m_materialBuffer->address();
        model->materialBufferRef.offsetInBytes = materialOffset;
        model->materialBufferRef.sizeInBytes = materialsSize;
        model->materialBufferRef.elementSize = sizeof(gpu::Material);

        auto materialPos = materialStart + materialOffset;
        for(const auto& modelMat: materials) {
            memcpy(materialPos, &modelMat->gpuMaterial, sizeof(gpu::Material));
            materialPos += sizeof(gpu::Material);
        }

        materialOffset += materialsSize;
    }

    m_materialBuffer->unmap();
}

void RenderSystem::beginFrame()
{
    m_framebufferIndex = m_swapchain->nextImageIndex(*m_imageAcquiredSemaphore);

    // Ensure command buffer is ready to use.
    vc.waitForFence(*m_commandBufferFence);

    vc.device->resetFences(*m_commandBufferFence);

    m_commandBuffer->begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
}

void RenderSystem::endFrame(std::vector<vk::Semaphore> waitSemaphores)
{
    waitSemaphores.push_back(*m_imageAcquiredSemaphore);
    std::vector<vk::PipelineStageFlags> pipeStageFlags(waitSemaphores.size(), vk::PipelineStageFlagBits::eAllCommands);

    vk::SubmitInfo submitInfo = {};
    submitInfo.setWaitSemaphoreCount((uint32_t)waitSemaphores.size());
    submitInfo.setPWaitSemaphores(waitSemaphores.data());
    submitInfo.setPWaitDstStageMask(pipeStageFlags.data());
    submitInfo.setCommandBufferCount(1);
    submitInfo.setPCommandBuffers(&*m_commandBuffer);
    submitInfo.setSignalSemaphoreCount(1);
    submitInfo.setPSignalSemaphores(&*m_renderCompleteSemaphore);

    m_commandBuffer->end();

    vc.graphicsQueue->submit(submitInfo, *m_commandBufferFence);
}

void RenderSystem::beginRenderPass()
{
    vk::RenderPassBeginInfo info = {};
    info.setRenderPass(*m_renderPass);
    info.setFramebuffer(m_swapchain->framebuffer(m_framebufferIndex));
    info.setRenderArea({{0, 0}, vc.windowSize});

    m_commandBuffer->beginRenderPass(info, vk::SubpassContents::eInline);
}

void RenderSystem::endRenderPass()
{
    m_commandBuffer->endRenderPass();
}

void RenderSystem::presentFrame()
{
    vk::PresentInfoKHR presentInfo = {};
    presentInfo.setWaitSemaphoreCount(1);
    presentInfo.setPWaitSemaphores(&*m_renderCompleteSemaphore);
    presentInfo.setSwapchainCount(1);
    presentInfo.setPSwapchains(&m_swapchain->swapchain());
    presentInfo.setPImageIndices(&m_framebufferIndex);

    try {
        vc.presentQueue->queue().presentKHR(presentInfo);
    }
    catch(const vk::OutOfDateKHRError&) {
        RAYGUN_DEBUG("Swap chain out of date");
        reload();
    }
}

void RenderSystem::setupRenderPass()
{
    std::array<vk::AttachmentDescription, 1> attachments;
    attachments[0].setFormat(vc.surfaceFormat);
    attachments[0].setSamples(SAMPLES);
    attachments[0].setLoadOp(vk::AttachmentLoadOp::eLoad);
    attachments[0].setInitialLayout(vk::ImageLayout::eTransferDstOptimal);
    attachments[0].setFinalLayout(vk::ImageLayout::ePresentSrcKHR);

    vk::AttachmentReference colorRef = {};
    colorRef.setAttachment(0);
    colorRef.setLayout(vk::ImageLayout::eColorAttachmentOptimal);

    std::array<vk::SubpassDescription, 1> subpasses;
    subpasses[0].setPipelineBindPoint(vk::PipelineBindPoint::eGraphics);
    subpasses[0].setColorAttachmentCount(1);
    subpasses[0].setPColorAttachments(&colorRef);

    std::array<vk::SubpassDependency, 1> dependencies = {};
    dependencies[0].setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
    dependencies[0].setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
    dependencies[0].setDstAccessMask(vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite);

    vk::RenderPassCreateInfo info = {};
    info.setAttachmentCount((uint32_t)attachments.size());
    info.setPAttachments(attachments.data());
    info.setSubpassCount((uint32_t)subpasses.size());
    info.setPSubpasses(subpasses.data());
    info.setDependencyCount((uint32_t)dependencies.size());
    info.setPDependencies(dependencies.data());

    m_renderPass = vc.device->createRenderPassUnique(info);
    vc.setObjectName(*m_renderPass, "Render System");
}

} // namespace raygun::render
