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

#include "raygun/render/raytracer.hpp"

#include <nv_helpers_vk/RaytracingPipelineGenerator.h>
#include <nv_helpers_vk/VKHelpers.h>

#include "raygun/entity.hpp"
#include "raygun/gpu/gpu_utils.hpp"
#include "raygun/logging.hpp"
#include "raygun/profiler.hpp"
#include "raygun/raygun.hpp"
#include "raygun/utils/array_utils.hpp"

#include "resources/shaders/compute_shader_shared.def"
#include "resources/shaders/raytracer_bindings.h"

namespace raygun::render {

void Raytracer::buildAccelerationStructure(Scene& scene)
{
    vc.waitForFence(*m_ASCommandBufferFence);
    vc.device->resetFences({*m_ASCommandBufferFence});
    m_ASBuildCommandBuffer->begin(vk::CommandBufferBeginInfo{});

    RG().profiler().resetQueries(*m_ASBuildCommandBuffer);

    RG().profiler().writeTimestamp(*m_ASBuildCommandBuffer, TimestampQueryID::ASBuildStart);

    // Create bottom level acceleration structures where needed.
    scene.root->forEachEntity([&](Entity& entity) {
        if(entity.model && !entity.model->bottomLevelAS)
            entity.model->bottomLevelAS = std::make_unique<BottomLevelAS>(*m_ASBuildCommandBuffer, *entity.model->mesh);
    });

    topLevelAS = std::make_unique<TopLevelAS>(*m_ASBuildCommandBuffer, scene);

    RG().profiler().writeTimestamp(*m_ASBuildCommandBuffer, TimestampQueryID::ASBuildEnd);

    m_ASBuildCommandBuffer->end();

    vc.computeQueue->submit(*m_ASBuildCommandBuffer, *m_ASCommandBufferFence, *topLevelASSemaphore);
}

void Raytracer::doRaytracing(vk::CommandBuffer& cmd)
{
    static int currentOutput = m_useFXAA ? 1 : 0;
    const char* names[] = {"Final", "Base/FXAA", "Normal", "Rough", "RTransition", "RCA", "RCB"};
    if(ImGui::Checkbox("Use FXAA", &m_useFXAA)) {
        currentOutput = m_useFXAA ? 1 : 0;
    }
    ImGui::Combo("Output Image", &currentOutput, names, RAYGUN_ARRAY_COUNT(names));
    gpu::Image* images[] = {&*m_finalImage, &*m_baseImage, &*m_normalImage, &*m_roughImage, &*m_roughTransitions, &*m_roughColorsA, &*m_roughColorsB};
    static_assert(RAYGUN_ARRAY_COUNT(names) == RAYGUN_ARRAY_COUNT(images));
    m_shownImage = images[currentOutput];

    cmd.bindPipeline(vk::PipelineBindPoint::eRayTracingNV, *rtPipeline);

    cmd.bindDescriptorSets(vk::PipelineBindPoint::eRayTracingNV, *rtPipelineLayout, 0, descriptorSet.set(), {});

    const VkDeviceSize rayGenOffset = rtSbt.GetRayGenOffset();
    const VkDeviceSize missOffset = rtSbt.GetMissOffset();
    const VkDeviceSize missStride = rtSbt.GetMissEntrySize();
    const VkDeviceSize hitGroupOffset = rtSbt.GetHitGroupOffset();
    const VkDeviceSize hitGroupStride = rtSbt.GetHitGroupEntrySize();

    RG().profiler().writeTimestamp(cmd, TimestampQueryID::RTTotalStart);

    RG().profiler().writeTimestamp(cmd, TimestampQueryID::RTOnlyStart);

    vkCmdTraceRaysNV(cmd, *sbtBuffer, rayGenOffset, *sbtBuffer, missOffset, missStride, *sbtBuffer, hitGroupOffset, hitGroupStride, VK_NULL_HANDLE, 0, 0,
                     vc.windowSize.width, vc.windowSize.height, 1);

    RG().profiler().writeTimestamp(cmd, TimestampQueryID::RTOnlyEnd);

    RG().profiler().writeTimestamp(cmd, TimestampQueryID::PostprocStart);

    int dispatchWidth = vc.windowSize.width / COMPUTE_WG_X_SIZE + ((vc.windowSize.width % COMPUTE_WG_X_SIZE) > 0 ? 1 : 0);
    int dispatchHeight = vc.windowSize.height / COMPUTE_WG_Y_SIZE + ((vc.windowSize.height % COMPUTE_WG_Y_SIZE) > 0 ? 1 : 0);

    RG().profiler().writeTimestamp(cmd, TimestampQueryID::RoughStart);
    m_roughPrepare->dispatch(cmd, dispatchWidth, dispatchHeight);
    for(int i = 0; i < 10; ++i) {
        m_roughBlurH->dispatch(cmd, dispatchWidth, dispatchHeight);
        m_roughBlurV->dispatch(cmd, dispatchWidth, dispatchHeight);
    }
    RG().profiler().writeTimestamp(cmd, TimestampQueryID::RoughEnd);

    m_postprocess->dispatch(cmd, dispatchWidth, dispatchHeight);

    if(m_useFXAA) {
        m_fxaa->dispatch(cmd, dispatchWidth, dispatchHeight);
    }
    RG().profiler().writeTimestamp(cmd, TimestampQueryID::PostprocEnd);

    RG().profiler().writeTimestamp(cmd, TimestampQueryID::RTTotalEnd);
}

void Raytracer::updateRenderTarget(const gpu::Buffer& uniformBuffer, const gpu::Buffer& vertexBuffer, const gpu::Buffer& indexBuffer,
                                   const gpu::Buffer& materialBuffer)
{
    // Bind acceleration structure
    descriptorSet.bind(RAYGUN_RAYTRACER_BINDING_ACCELERATION_STRUCTURE, *topLevelAS);

    // Bind images
    descriptorSet.bind(RAYGUN_RAYTRACER_BINDING_OUTPUT_IMAGE, *m_baseImage);
    descriptorSet.bind(RAYGUN_RAYTRACER_BINDING_ROUGH_IMAGE, *m_roughImage);
    descriptorSet.bind(RAYGUN_RAYTRACER_BINDING_NORMAL_IMAGE, *m_normalImage);

    // Bind buffers
    descriptorSet.bind(RAYGUN_RAYTRACER_BINDING_UNIFORM_BUFFER, uniformBuffer);
    descriptorSet.bind(RAYGUN_RAYTRACER_BINDING_VERTEX_BUFFER, vertexBuffer);
    descriptorSet.bind(RAYGUN_RAYTRACER_BINDING_INDEX_BUFFER, indexBuffer);
    descriptorSet.bind(RAYGUN_RAYTRACER_BINDING_MATERIAL_BUFFER, materialBuffer);
    descriptorSet.bind(RAYGUN_RAYTRACER_BINDING_INSTANCE_OFFSET_TABLE, topLevelAS->instanceOffsetTable());

    descriptorSet.update();

    RG().computeSystem().updateDescriptors(
        uniformBuffer, {&*m_finalImage, &*m_baseImage, &*m_normalImage, &*m_roughImage, &*m_roughTransitions, &*m_roughColorsA, &*m_roughColorsB});
}

void Raytracer::imageShaderWriteBarrier(vk::CommandBuffer& cmd, vk::Image& image)
{
    VkImageSubresourceRange subresourceRange;
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = 1;
    subresourceRange.baseArrayLayer = 0;
    subresourceRange.layerCount = 1;

    nv_helpers_vk::imageBarrier(cmd, image, subresourceRange, 0, VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
}

Raytracer::Raytracer() : vc(RG().vc())
{
    setupRaytracingImages();

    setupRaytracing();

    setupRaytracingDescriptorSet();

    setupRaytracingPipeline();

    setupShaderBindingTable();

    setupPostprocessing();

    RAYGUN_INFO("Raytracer initialized");
}

Raytracer::~Raytracer()
{
    vc.device->waitIdle();
}

void Raytracer::resize()
{
    setupRaytracingImages();

    setupRaytracing();

    setupRaytracingPipeline();

    setupShaderBindingTable();
}

void Raytracer::reload()
{
    RAYGUN_INFO("Raytracer reload");

    RG().resourceManager().clearShaderCache();

    vc.waitIdle();

    // recompile shaders
    {
        const auto shaderDir = fs::path{"resources/shaders"};
        const std::set<fs::path> extensions = {".comp", ".rgen", ".rchit", ".rmiss"};

        for(const auto& entry: fs::directory_iterator(shaderDir)) {
            if(extensions.find(entry.path().extension()) == extensions.end()) continue;

            const auto cmd = fmt::format("glslc.exe -Isrc -o {1}.spv {0}", entry.path(), shaderDir / entry.path().stem());

            if(system(cmd.c_str()) != 0) {
                RAYGUN_WARN("Compiling {} failed", entry.path());
            }
            else {
                RAYGUN_INFO("Compiled {}", entry.path());
            }
        }
    }

    setupRaytracingPipeline();

    setupShaderBindingTable();

    setupPostprocessing();
}

void Raytracer::setupRaytracingImages()
{
    m_baseImage = std::make_unique<gpu::Image>(vc.windowSize);
    m_normalImage = std::make_unique<gpu::Image>(vc.windowSize);
    m_roughImage = std::make_unique<gpu::Image>(vc.windowSize);
    m_finalImage = std::make_unique<gpu::Image>(vc.windowSize);

    m_roughTransitions = std::make_unique<gpu::Image>(vc.windowSize, vk::Format::eR8Snorm);
    m_roughColorsA = std::make_unique<gpu::Image>(vc.windowSize);
    m_roughColorsB = std::make_unique<gpu::Image>(vc.windowSize);
}

void Raytracer::setupRaytracing()
{
    vk::PhysicalDeviceProperties2 props = {};
    props.pNext = static_cast<void*>(&raytracingProperties);
    vc.physicalDevice.getProperties2(&props);

    topLevelASSemaphore = vc.device->createSemaphoreUnique({});
    m_ASBuildCommandBuffer = vc.computeQueue->createCommandBuffer();
    m_ASCommandBufferFence = vc.device->createFenceUnique({vk::FenceCreateFlagBits::eSignaled});
}

void Raytracer::setupRaytracingDescriptorSet()
{
    descriptorSet.addBinding(RAYGUN_RAYTRACER_BINDING_ACCELERATION_STRUCTURE, 1, vk::DescriptorType::eAccelerationStructureNV,
                             vk::ShaderStageFlagBits::eRaygenNV | vk::ShaderStageFlagBits::eClosestHitNV);

    descriptorSet.addBinding(RAYGUN_RAYTRACER_BINDING_OUTPUT_IMAGE, 1, vk::DescriptorType::eStorageImage, vk::ShaderStageFlagBits::eRaygenNV);
    descriptorSet.addBinding(RAYGUN_RAYTRACER_BINDING_ROUGH_IMAGE, 1, vk::DescriptorType::eStorageImage, vk::ShaderStageFlagBits::eRaygenNV);
    descriptorSet.addBinding(RAYGUN_RAYTRACER_BINDING_NORMAL_IMAGE, 1, vk::DescriptorType::eStorageImage, vk::ShaderStageFlagBits::eRaygenNV);

    descriptorSet.addBinding(RAYGUN_RAYTRACER_BINDING_UNIFORM_BUFFER, 1, vk::DescriptorType::eUniformBuffer,
                             vk::ShaderStageFlagBits::eRaygenNV | vk::ShaderStageFlagBits::eClosestHitNV | vk::ShaderStageFlagBits::eMissNV);
    descriptorSet.addBinding(RAYGUN_RAYTRACER_BINDING_VERTEX_BUFFER, 1, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eClosestHitNV);
    descriptorSet.addBinding(RAYGUN_RAYTRACER_BINDING_INDEX_BUFFER, 1, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eClosestHitNV);
    descriptorSet.addBinding(RAYGUN_RAYTRACER_BINDING_MATERIAL_BUFFER, 1, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eClosestHitNV);

    descriptorSet.addBinding(RAYGUN_RAYTRACER_BINDING_INSTANCE_OFFSET_TABLE, 1, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eClosestHitNV);

    descriptorSet.generate();
}

void Raytracer::setupRaytracingPipeline()
{
    nv_helpers_vk::RayTracingPipelineGenerator gen;

    const auto rayGen = RG().resourceManager().loadShader("raygen");
    rayGenIndex = gen.AddRayGenShaderStage(*rayGen->shaderModule);

    const auto miss = RG().resourceManager().loadShader("miss");
    missIndex = gen.AddMissShaderStage(*miss->shaderModule);

    const auto shadowMiss = RG().resourceManager().loadShader("shadowMiss");
    shadowMissIndex = gen.AddMissShaderStage(*shadowMiss->shaderModule);

    hitGroupIndex = gen.StartHitGroup();
    const auto closestHit = RG().resourceManager().loadShader("closesthit");
    gen.AddHitShaderStage(*closestHit->shaderModule, VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV);
    gen.EndHitGroup();

    shadowHitGroupIndex = gen.StartHitGroup();
    gen.EndHitGroup();

    gen.SetMaxRecursionDepth(7);

    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;

    gen.Generate(*vc.device, descriptorSet.layout(), &pipeline, &pipelineLayout);

    rtPipeline = gpu::wrapUnique<vk::Pipeline>(pipeline, *vc.device);
    rtPipelineLayout = gpu::wrapUnique<vk::PipelineLayout>(pipelineLayout, *vc.device);
}

void Raytracer::setupShaderBindingTable()
{
    // Reset generator
    rtSbt = nv_helpers_vk::ShaderBindingTableGenerator{};

    rtSbt.AddRayGenerationProgram(rayGenIndex, {});

    rtSbt.AddMissProgram(missIndex, {});

    rtSbt.AddMissProgram(shadowMissIndex, {});

    rtSbt.AddHitGroup(hitGroupIndex, {});

    rtSbt.AddHitGroup(shadowHitGroupIndex, {});

    const auto sbtSize = rtSbt.ComputeSBTSize(raytracingProperties);

    sbtBuffer = std::make_unique<gpu::Buffer>(sbtSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible);

    rtSbt.Generate(*vc.device, *rtPipeline, sbtBuffer->memory());
}

void Raytracer::setupPostprocessing()
{
    auto& cs = RG().computeSystem();

    m_roughPrepare = cs.createComputePass("rough_prepare");
    m_roughBlurH = cs.createComputePass("rough_blur_h");
    m_roughBlurV = cs.createComputePass("rough_blur_v");

    m_postprocess = cs.createComputePass("postprocess");
    m_fxaa = cs.createComputePass("fxaa");
}

} // namespace raygun::render
