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

#include "raygun/entity.hpp"
#include "raygun/gpu/gpu_utils.hpp"
#include "raygun/logging.hpp"
#include "raygun/profiler.hpp"
#include "raygun/raygun.hpp"
#include "raygun/utils/array_utils.hpp"

#include "resources/shaders/compute_shader_shared.def"
#include "resources/shaders/raytracer_bindings.h"

namespace raygun::render {

Raytracer::Raytracer() : vc(RG().vc())
{
    auto properties = vc.physicalDevice.getProperties2<vk::PhysicalDeviceProperties2, vk::PhysicalDeviceRayTracingPropertiesKHR>();
    m_properties = properties.get<vk::PhysicalDeviceRayTracingPropertiesKHR>();

    setupRaytracingDescriptorSet();

    setupPostprocessing();

    setupRaytracingImages();

    setupRaytracingPipeline();

    setupShaderBindingTable();

    RAYGUN_INFO("Raytracer initialized");
}

void Raytracer::setupBottomLevelAS()
{
    auto cmd = vc.computeQueue->createCommandBuffer();
    vc.setObjectName(*cmd, "BLAS");

    auto fence = vc.device->createFenceUnique({});
    vc.setObjectName(*fence, "BLAS");

    cmd->begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});

    auto models = RG().resourceManager().models();
    for(auto& model: models) {
        if(!model->bottomLevelAS) {
            model->bottomLevelAS = std::make_unique<BottomLevelAS>(*cmd, *model->mesh);
        }
    }

    cmd->end();
    vc.computeQueue->submit(*cmd, *fence);
    vc.waitForFence(*fence);
}

void Raytracer::setupTopLevelAS(vk::CommandBuffer& cmd, const Scene& scene)
{
    RG().profiler().writeTimestamp(cmd, TimestampQueryID::ASBuildStart);

    m_topLevelAS = std::make_unique<TopLevelAS>(cmd, scene);

    accelerationStructureBarrier(cmd);

    RG().profiler().writeTimestamp(cmd, TimestampQueryID::ASBuildEnd);
}

const gpu::Image& Raytracer::doRaytracing(vk::CommandBuffer& cmd)
{
    cmd.bindPipeline(vk::PipelineBindPoint::eRayTracingKHR, *m_pipeline);

    cmd.bindDescriptorSets(vk::PipelineBindPoint::eRayTracingKHR, *m_pipelineLayout, 0, m_descriptorSet.set(), {});

    RG().profiler().writeTimestamp(cmd, TimestampQueryID::RTTotalStart);

    RG().profiler().writeTimestamp(cmd, TimestampQueryID::RTOnlyStart);

    initialImageBarrier(cmd);

    cmd.traceRaysKHR(m_raygenSbt, m_missSbt, m_hitSbt, m_callableSbt, //
                     vc.windowSize.width, vc.windowSize.height, 1);

    computeShaderImageBarrier(cmd, {m_baseImage.get(), m_normalImage.get(), m_roughImage.get()}, vk::PipelineStageFlagBits::eRayTracingShaderKHR);

    RG().profiler().writeTimestamp(cmd, TimestampQueryID::RTOnlyEnd);

    RG().profiler().writeTimestamp(cmd, TimestampQueryID::PostprocStart);

    int dispatchWidth = vc.windowSize.width / COMPUTE_WG_X_SIZE + ((vc.windowSize.width % COMPUTE_WG_X_SIZE) > 0 ? 1 : 0);
    int dispatchHeight = vc.windowSize.height / COMPUTE_WG_Y_SIZE + ((vc.windowSize.height % COMPUTE_WG_Y_SIZE) > 0 ? 1 : 0);

    RG().profiler().writeTimestamp(cmd, TimestampQueryID::RoughStart);

    m_roughPrepare->dispatch(cmd, dispatchWidth, dispatchHeight);
    computeShaderImageBarrier(cmd, {m_roughTransitions.get(), m_roughColorsA.get(), m_roughColorsB.get()});

    for(int i = 0; i < 10; ++i) {
        m_roughBlurH->dispatch(cmd, dispatchWidth, dispatchHeight);
        computeShaderImageBarrier(cmd, {m_roughColorsA.get(), m_roughColorsB.get()});
        m_roughBlurV->dispatch(cmd, dispatchWidth, dispatchHeight);
        computeShaderImageBarrier(cmd, {m_roughColorsA.get(), m_roughColorsB.get()});
    }

    RG().profiler().writeTimestamp(cmd, TimestampQueryID::RoughEnd);

    m_postprocess->dispatch(cmd, dispatchWidth, dispatchHeight);
    computeShaderImageBarrier(cmd, {
                                       m_baseImage.get(),
                                       m_normalImage.get(),
                                       m_roughImage.get(),
                                       m_finalImage.get(),
                                       m_roughTransitions.get(),
                                       m_roughColorsA.get(),
                                       m_roughColorsB.get(),
                                   });

    ImGui::Checkbox("Use FXAA", &m_useFXAA);
    if(m_useFXAA) {
        m_fxaa->dispatch(cmd, dispatchWidth, dispatchHeight);
        std::swap(m_baseImage, m_finalImage);
    }

    RG().profiler().writeTimestamp(cmd, TimestampQueryID::PostprocEnd);

    RG().profiler().writeTimestamp(cmd, TimestampQueryID::RTTotalEnd);

    return selectResultImage();
}

void Raytracer::updateRenderTarget(const gpu::Buffer& uniformBuffer, const gpu::Buffer& vertexBuffer, const gpu::Buffer& indexBuffer,
                                   const gpu::Buffer& materialBuffer)
{
    // Bind acceleration structure
    m_descriptorSet.bind(RAYGUN_RAYTRACER_BINDING_ACCELERATION_STRUCTURE, *m_topLevelAS);

    // Bind images
    m_descriptorSet.bind(RAYGUN_RAYTRACER_BINDING_OUTPUT_IMAGE, *m_baseImage);
    m_descriptorSet.bind(RAYGUN_RAYTRACER_BINDING_ROUGH_IMAGE, *m_roughImage);
    m_descriptorSet.bind(RAYGUN_RAYTRACER_BINDING_NORMAL_IMAGE, *m_normalImage);

    // Bind buffers
    m_descriptorSet.bind(RAYGUN_RAYTRACER_BINDING_UNIFORM_BUFFER, uniformBuffer);
    m_descriptorSet.bind(RAYGUN_RAYTRACER_BINDING_VERTEX_BUFFER, vertexBuffer);
    m_descriptorSet.bind(RAYGUN_RAYTRACER_BINDING_INDEX_BUFFER, indexBuffer);
    m_descriptorSet.bind(RAYGUN_RAYTRACER_BINDING_MATERIAL_BUFFER, materialBuffer);
    m_descriptorSet.bind(RAYGUN_RAYTRACER_BINDING_INSTANCE_OFFSET_TABLE, m_topLevelAS->instanceOffsetTable());

    m_descriptorSet.update();

    RG().computeSystem().updateDescriptors(
        uniformBuffer, {&*m_finalImage, &*m_baseImage, &*m_normalImage, &*m_roughImage, &*m_roughTransitions, &*m_roughColorsA, &*m_roughColorsB});
}

void Raytracer::setupRaytracingImages()
{
    m_baseImage = std::make_unique<gpu::Image>(vc.windowSize);
    m_baseImage->setName("RT Base Image");

    m_normalImage = std::make_unique<gpu::Image>(vc.windowSize);
    m_normalImage->setName("RT Normal Image");

    m_roughImage = std::make_unique<gpu::Image>(vc.windowSize);
    m_roughImage->setName("RT Rough Image");

    m_finalImage = std::make_unique<gpu::Image>(vc.windowSize);
    m_finalImage->setName("RT Final Image");

    m_roughTransitions = std::make_unique<gpu::Image>(vc.windowSize, vk::Format::eR8Snorm);
    m_roughTransitions->setName("RT Rough Transition");

    m_roughColorsA = std::make_unique<gpu::Image>(vc.windowSize);
    m_roughColorsA->setName("RT Rough Color A");

    m_roughColorsB = std::make_unique<gpu::Image>(vc.windowSize);
    m_roughColorsB->setName("RT Rough Color B");
}

void Raytracer::setupRaytracingDescriptorSet()
{
    m_descriptorSet.setName("Ray Tracer");

    m_descriptorSet.addBinding(RAYGUN_RAYTRACER_BINDING_ACCELERATION_STRUCTURE, 1, vk::DescriptorType::eAccelerationStructureKHR,
                               vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR);

    m_descriptorSet.addBinding(RAYGUN_RAYTRACER_BINDING_OUTPUT_IMAGE, 1, vk::DescriptorType::eStorageImage, vk::ShaderStageFlagBits::eRaygenKHR);
    m_descriptorSet.addBinding(RAYGUN_RAYTRACER_BINDING_ROUGH_IMAGE, 1, vk::DescriptorType::eStorageImage, vk::ShaderStageFlagBits::eRaygenKHR);
    m_descriptorSet.addBinding(RAYGUN_RAYTRACER_BINDING_NORMAL_IMAGE, 1, vk::DescriptorType::eStorageImage, vk::ShaderStageFlagBits::eRaygenKHR);

    m_descriptorSet.addBinding(RAYGUN_RAYTRACER_BINDING_UNIFORM_BUFFER, 1, vk::DescriptorType::eUniformBuffer,
                               vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eMissKHR);
    m_descriptorSet.addBinding(RAYGUN_RAYTRACER_BINDING_VERTEX_BUFFER, 1, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eClosestHitKHR);
    m_descriptorSet.addBinding(RAYGUN_RAYTRACER_BINDING_INDEX_BUFFER, 1, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eClosestHitKHR);
    m_descriptorSet.addBinding(RAYGUN_RAYTRACER_BINDING_MATERIAL_BUFFER, 1, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eClosestHitKHR);

    m_descriptorSet.addBinding(RAYGUN_RAYTRACER_BINDING_INSTANCE_OFFSET_TABLE, 1, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eClosestHitKHR);

    m_descriptorSet.generate();
}

namespace {

    vk::RayTracingShaderGroupCreateInfoKHR generalShaderGroup(uint32_t generalShaderIndex)
    {
        vk::RayTracingShaderGroupCreateInfoKHR info;
        info.setGeneralShader(generalShaderIndex);
        info.setClosestHitShader(VK_SHADER_UNUSED_KHR);
        info.setAnyHitShader(VK_SHADER_UNUSED_KHR);
        info.setIntersectionShader(VK_SHADER_UNUSED_KHR);

        return info;
    }

    vk::RayTracingShaderGroupCreateInfoKHR HitShaderGroup(uint32_t closestHitShaderIndex)
    {
        vk::RayTracingShaderGroupCreateInfoKHR info{vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup};
        info.setGeneralShader(VK_SHADER_UNUSED_KHR);
        info.setClosestHitShader(closestHitShaderIndex);
        info.setAnyHitShader(VK_SHADER_UNUSED_KHR);
        info.setIntersectionShader(VK_SHADER_UNUSED_KHR);

        return info;
    }

} // namespace

namespace {

    vk::RayTracingShaderGroupCreateInfoKHR generalShaderGroupInfo(uint32_t index)
    {
        vk::RayTracingShaderGroupCreateInfoKHR info = {};
        info.setGeneralShader(index);
        info.setClosestHitShader(VK_SHADER_UNUSED_KHR);
        info.setAnyHitShader(VK_SHADER_UNUSED_KHR);
        info.setIntersectionShader(VK_SHADER_UNUSED_KHR);

        return info;
    }

    vk::RayTracingShaderGroupCreateInfoKHR closestHitShaderGroupInfo(uint32_t index)
    {
        vk::RayTracingShaderGroupCreateInfoKHR info = {vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup};
        info.setGeneralShader(VK_SHADER_UNUSED_KHR);
        info.setClosestHitShader(index);
        info.setAnyHitShader(VK_SHADER_UNUSED_KHR);
        info.setIntersectionShader(VK_SHADER_UNUSED_KHR);

        return info;
    }

} // namespace

void Raytracer::setupRaytracingPipeline()
{
    const auto sbtStride = m_properties.shaderGroupBaseAlignment;

    std::vector<vk::PipelineShaderStageCreateInfo> stages;
    std::vector<vk::RayTracingShaderGroupCreateInfoKHR> groups;

    // raygen group
    const auto raygenShader = RG().resourceManager().loadShader("raygen.rgen");
    {
        m_raygenSbt.setOffset(groups.size() * sbtStride);

        stages.push_back(raygenShader->shaderStageInfo(vk::ShaderStageFlagBits::eRaygenKHR));
        groups.push_back(generalShaderGroupInfo((uint32_t)groups.size()));
    }

    // miss group
    const auto missShader = RG().resourceManager().loadShader("miss.rmiss");
    const auto shadowMissShader = RG().resourceManager().loadShader("shadowMiss.rmiss");
    {
        m_missSbt.setOffset(groups.size() * sbtStride);

        stages.push_back(missShader->shaderStageInfo(vk::ShaderStageFlagBits::eMissKHR));
        groups.push_back(generalShaderGroupInfo((uint32_t)groups.size()));

        stages.push_back(shadowMissShader->shaderStageInfo(vk::ShaderStageFlagBits::eMissKHR));
        groups.push_back(generalShaderGroupInfo((uint32_t)groups.size()));
    }

    // hit group
    const auto closestHitShader = RG().resourceManager().loadShader("closesthit.rchit");
    {
        m_hitSbt.setOffset(groups.size() * sbtStride);

        stages.push_back(closestHitShader->shaderStageInfo(vk::ShaderStageFlagBits::eClosestHitKHR));
        groups.push_back(closestHitShaderGroupInfo((uint32_t)groups.size()));
    }

    const auto sbtSize = groups.size() * sbtStride;

    m_raygenSbt.setStride(sbtStride).setSize(sbtSize);
    m_missSbt.setStride(sbtStride).setSize(sbtSize);
    m_hitSbt.setStride(sbtStride).setSize(sbtSize);
    m_callableSbt.setStride(sbtStride).setSize(sbtSize);

    {
        vk::PipelineLayoutCreateInfo info = {};
        info.setSetLayoutCount(1);
        info.setPSetLayouts(&m_descriptorSet.layout());

        m_pipelineLayout = vc.device->createPipelineLayoutUnique(info);
        vc.setObjectName(*m_pipelineLayout, "Ray Tracer");
    }

    {
        vk::RayTracingPipelineCreateInfoKHR info = {};
        info.setStageCount((uint32_t)stages.size());
        info.setPStages(stages.data());
        info.setGroupCount((uint32_t)groups.size());
        info.setPGroups(groups.data());
        info.setMaxRecursionDepth(7);
        info.setLayout(*m_pipelineLayout);

        // FIXME: Vulkan SDK >= 1.2.136
        // https://github.com/KhronosGroup/Vulkan-Hpp/issues/557
        {
            const auto vkCreateRayTracingPipelinesKHR =
                reinterpret_cast<PFN_vkCreateRayTracingPipelinesKHR>(vc.device->getProcAddr("vkCreateRayTracingPipelinesKHR"));
            RAYGUN_ASSERT(vkCreateRayTracingPipelinesKHR);

            VkPipeline pipeline;
            const auto result = vkCreateRayTracingPipelinesKHR(*vc.device, nullptr, 1, &VkRayTracingPipelineCreateInfoKHR(info), nullptr, &pipeline);
            RAYGUN_ASSERT(result == VK_SUCCESS);

            m_pipeline = gpu::wrapUnique<vk::Pipeline>(pipeline, *vc.device);
        }
        vc.setObjectName(*m_pipeline, "Ray Tracer");
    }
}

void Raytracer::setupShaderBindingTable()
{
    const auto sbtSize = m_raygenSbt.size;
    const auto groupCount = (uint32_t)(m_raygenSbt.size / m_raygenSbt.stride);

    std::vector<uint8_t> groupHandles(groupCount * m_properties.shaderGroupHandleSize);
    vc.device->getRayTracingShaderGroupHandlesKHR(*m_pipeline, 0, groupCount, groupHandles.size(), groupHandles.data());

    m_sbtBuffer = std::make_unique<gpu::Buffer>(sbtSize, vk::BufferUsageFlagBits::eRayTracingKHR, vk::MemoryPropertyFlagBits::eHostVisible);
    m_sbtBuffer->setName("Shader Binding Table");

    // Shader group handles should be aligned according to
    // shaderGroupBaseAlignment. The handles we get from
    // getRayTracingShaderGroupHandlesKHR are consecutive in memory
    // (shaderGroupHandleSize), hence we need to copy them over adjusting the
    // alignment.
    {
        auto p = reinterpret_cast<uint8_t*>(m_sbtBuffer->map());
        for(auto i = 0u; i < groupCount; i++) {
            memcpy(p, groupHandles.data() + i * m_properties.shaderGroupHandleSize, m_properties.shaderGroupHandleSize);
            p += m_properties.shaderGroupBaseAlignment;
        }

        m_sbtBuffer->unmap();
    }

    m_raygenSbt.setBuffer(*m_sbtBuffer);
    m_missSbt.setBuffer(*m_sbtBuffer);
    m_hitSbt.setBuffer(*m_sbtBuffer);
    m_callableSbt.setBuffer(*m_sbtBuffer);
}

void Raytracer::setupPostprocessing()
{
    auto& cs = RG().computeSystem();

    m_roughPrepare = cs.createComputePass("rough_prepare.comp");
    m_roughBlurH = cs.createComputePass("rough_blur_h.comp");
    m_roughBlurV = cs.createComputePass("rough_blur_v.comp");

    m_postprocess = cs.createComputePass("postprocess.comp");
    m_fxaa = cs.createComputePass("fxaa.comp");
}

const gpu::Image& Raytracer::selectResultImage()
{
    // For debugging purposes the result image can be selected via ImGui.

    const char* imageNames[] = {"Final", "Base/Temp", "Normal", "Rough", "RTransition", "RCA", "RCB"};
    gpu::Image* images[] = {m_finalImage.get(),       m_baseImage.get(),    m_normalImage.get(), m_roughImage.get(),
                            m_roughTransitions.get(), m_roughColorsA.get(), m_roughColorsB.get()};
    static_assert(RAYGUN_ARRAY_COUNT(imageNames) == RAYGUN_ARRAY_COUNT(images));

    static int selectedResult = 0;
    ImGui::Combo("Image", &selectedResult, imageNames, RAYGUN_ARRAY_COUNT(imageNames));

    return *images[selectedResult];
}

void Raytracer::initialImageBarrier(vk::CommandBuffer& cmd)
{
    const auto images = {m_baseImage.get(),        m_normalImage.get(),  m_roughImage.get(),  m_finalImage.get(),
                         m_roughTransitions.get(), m_roughColorsA.get(), m_roughColorsB.get()};

    std::vector<vk::ImageMemoryBarrier> imageBarriers;
    imageBarriers.reserve(images.size());
    for(auto& image: images) {
        auto& barrier = imageBarriers.emplace_back();
        barrier.setImage(*image);
        barrier.setOldLayout(vk::ImageLayout::eUndefined);
        barrier.setNewLayout(vk::ImageLayout::eGeneral);
        barrier.setSubresourceRange(gpu::defaultImageSubresourceRange());
    }

    cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eRayTracingShaderKHR, //
                        vk::DependencyFlagBits::eByRegion, {}, {}, imageBarriers);
}

void Raytracer::computeShaderImageBarrier(vk::CommandBuffer& cmd, std::initializer_list<gpu::Image*> images, vk::PipelineStageFlags srcStageMask)
{
    std::vector<vk::ImageMemoryBarrier> imageBarriers;
    imageBarriers.reserve(images.size());
    for(auto& image: images) {
        auto& barrier = imageBarriers.emplace_back();
        barrier.setImage(*image);
        barrier.setSrcAccessMask(vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite);
        barrier.setDstAccessMask(vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite);
        barrier.setOldLayout(vk::ImageLayout::eGeneral);
        barrier.setNewLayout(vk::ImageLayout::eGeneral);
        barrier.setSubresourceRange(gpu::defaultImageSubresourceRange());
    }

    cmd.pipelineBarrier(srcStageMask, vk::PipelineStageFlagBits::eComputeShader, //
                        vk::DependencyFlagBits::eByRegion, {}, {}, imageBarriers);
}

} // namespace raygun::render
