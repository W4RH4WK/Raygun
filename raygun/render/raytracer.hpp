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

#include "raygun/compute/compute_system.hpp"
#include "raygun/gpu/descriptor_set.hpp"
#include "raygun/gpu/gpu_buffer.hpp"
#include "raygun/gpu/image.hpp"
#include "raygun/render/acceleration_structure.hpp"
#include "raygun/scene.hpp"
#include "raygun/vulkan_context.hpp"

namespace raygun::render {

/// Renderer which is responsible for ray tracing.
struct Raytracer {
    Raytracer();

    void setupBottomLevelAS();

    void setupTopLevelAS(vk::CommandBuffer& cmd, const Scene& scene);

    const gpu::Image& doRaytracing(vk::CommandBuffer& cmd);

    void updateRenderTarget(const gpu::Buffer& uniformBuffer, const gpu::Buffer& vertexBuffer, const gpu::Buffer& indexBuffer,
                            const gpu::Buffer& materialBuffer);

  private:
    void setupRaytracingImages();

    void setupRaytracingDescriptorSet();

    void setupRaytracingPipeline();

    void setupPostprocessing();

    const gpu::Image& selectResultImage();

    void initialImageBarrier(vk::CommandBuffer& cmd);

    void computeShaderImageBarrier(vk::CommandBuffer& cmd, std::initializer_list<gpu::Image*> images,
                                   vk::PipelineStageFlags srcStageMask = vk::PipelineStageFlagBits::eComputeShader);

    vk::PhysicalDeviceRayTracingPipelinePropertiesKHR m_properties = {};

    UniqueTopLevelAS m_topLevelAS;

    gpu::DescriptorSet m_descriptorSet;

    vk::StridedDeviceAddressRegionKHR m_raygenSbt = {};
    vk::StridedDeviceAddressRegionKHR m_missSbt = {};
    vk::StridedDeviceAddressRegionKHR m_hitSbt = {};
    vk::StridedDeviceAddressRegionKHR m_callableSbt = {};

    vk::UniquePipeline m_pipeline;
    vk::UniquePipelineLayout m_pipelineLayout;

    gpu::UniqueBuffer m_sbtBuffer;

    bool m_useFXAA = true;
    compute::UniqueComputePass m_postprocess;
    compute::UniqueComputePass m_fxaa;

    compute::UniqueComputePass m_roughPrepare;
    compute::UniqueComputePass m_roughBlurH;
    compute::UniqueComputePass m_roughBlurV;

    // these are rendered to directly in ray tracing
    gpu::UniqueImage m_baseImage;
    gpu::UniqueImage m_normalImage;
    gpu::UniqueImage m_roughImage;

    // final image storage
    gpu::UniqueImage m_finalImage;

    // intermediate buffers
    gpu::UniqueImage m_roughTransitions;
    gpu::UniqueImage m_roughColorsA, m_roughColorsB;

    VulkanContext& vc;
};

using UniqueRaytracer = std::unique_ptr<Raytracer>;

} // namespace raygun::render
