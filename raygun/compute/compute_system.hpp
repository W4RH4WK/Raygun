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

#include "raygun/vulkan_context.hpp"

#include "raygun/gpu/descriptor_set.hpp"
#include "raygun/gpu/gpu_buffer.hpp"
#include "raygun/gpu/image.hpp"
#include "raygun/gpu/shader.hpp"

namespace raygun::compute {

class ComputeSystem;

class ComputePass {
  public:
    void dispatch(vk::CommandBuffer& cmd, uint32_t width, uint32_t height = 1, uint32_t depth = 1);

  private:
    ComputePass(string_view name);

    std::shared_ptr<gpu::Shader> computeShader;
    vk::UniquePipeline computePipeline;

    ComputeSystem& cs;

    friend class ComputeSystem;
};

using UniqueComputePass = std::unique_ptr<ComputePass>;

class ComputeSystem {
  public:
    ComputeSystem();

    void updateDescriptors(const gpu::Buffer& ubo, std::initializer_list<gpu::Image*> images);

    UniqueComputePass createComputePass(string_view name);

  private:
    static constexpr int PRE_IMG_ELEMENTS = 1;
    static constexpr int NUM_IMAGES = 7;
    static constexpr int NUM_MIP_IMAGES = 0;

    void bindDescriptorSet(vk::CommandBuffer& cmd);

    gpu::DescriptorSet descriptorSet;

    vk::UniquePipelineLayout computePipelineLayout;

    vk::UniqueSampler linearClampedSampler;

    VulkanContext& vc;

    friend class ComputePass;
};

using UniqueComputeSystem = std::unique_ptr<ComputeSystem>;

} // namespace raygun::compute
