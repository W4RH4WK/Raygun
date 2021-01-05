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

#include "raygun/compute/compute_system.hpp"

#include "raygun/gpu/gpu_utils.hpp"
#include "raygun/profiler.hpp"
#include "raygun/raygun.hpp"
#include "raygun/render/raytracer.hpp"
#include "raygun/utils/array_utils.hpp"

#include "resources/shaders/compute_shader_shared.def"

namespace raygun::compute {

void ComputePass::dispatch(vk::CommandBuffer& cmd, uint32_t width, uint32_t height, uint32_t depth)
{
    cmd.bindPipeline(vk::PipelineBindPoint::eCompute, *computePipeline);
    cs.bindDescriptorSet(cmd);
    cmd.dispatch(width, height, depth);
}

ComputePass::ComputePass(string_view name) : computeShader(RG().resourceManager().loadShader(name)), cs(RG().computeSystem())
{
    auto shaderStageInfo = computeShader->shaderStageInfo(vk::ShaderStageFlagBits::eCompute);

    vk::ComputePipelineCreateInfo pipeInfo;
    pipeInfo.setLayout(*cs.computePipelineLayout);
    pipeInfo.setStage(shaderStageInfo);

    computePipeline = cs.vc.device->createComputePipelineUnique(nullptr, pipeInfo).value;
    RG().vc().setObjectName(*computePipeline, name);

    RAYGUN_TRACE("Compute pass {} initialized", name);
}

ComputeSystem::ComputeSystem() : vc(RG().vc())
{
    descriptorSet.setName("Compute System");
    descriptorSet.addBinding(0, 1, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eCompute);

    for(auto i = 1; i < PRE_IMG_ELEMENTS + NUM_IMAGES * 2; i += 2) {
        uint32_t mipMaps = i >= PRE_IMG_ELEMENTS + (NUM_IMAGES - NUM_MIP_IMAGES) * 2 ? COMPUTE_PP_MIPS : 1;
        descriptorSet.addBinding(i + 0, mipMaps, vk::DescriptorType::eStorageImage, vk::ShaderStageFlagBits::eCompute);
        descriptorSet.addBinding(i + 1, 1, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eCompute);
    }

    descriptorSet.generate();

    vk::PipelineLayoutCreateInfo layoutInfo;
    layoutInfo.setSetLayoutCount(1);
    layoutInfo.setPSetLayouts(&descriptorSet.layout());

    computePipelineLayout = vc.device->createPipelineLayoutUnique(layoutInfo);
    vc.setObjectName(*computePipelineLayout, "Compute System");

    vk::SamplerCreateInfo samplerInfo;
    samplerInfo.setAddressModeU(vk::SamplerAddressMode::eClampToEdge);
    samplerInfo.setAddressModeV(vk::SamplerAddressMode::eClampToEdge);
    samplerInfo.setAddressModeW(vk::SamplerAddressMode::eClampToEdge);
    samplerInfo.setMagFilter(vk::Filter::eLinear);
    samplerInfo.setMinFilter(vk::Filter::eLinear);
    samplerInfo.setMipmapMode(vk::SamplerMipmapMode::eLinear);
    samplerInfo.setMaxLod(100); // this is perfectly legal
    linearClampedSampler = vc.device->createSamplerUnique(samplerInfo);
    vc.setObjectName(*linearClampedSampler, "Compute System Linear Clamped Sampler");

    RAYGUN_INFO("Compute system initialized");
}

void ComputeSystem::updateDescriptors(const gpu::Buffer& ubo, std::initializer_list<gpu::Image*> images)
{
    RAYGUN_ASSERT(images.size() == NUM_IMAGES);

    uint32_t bindIndex = 0;

    descriptorSet.bind(bindIndex++, ubo);

    uint32_t samplerIndex = 0;
    vk::DescriptorImageInfo samplerInfos[NUM_IMAGES] = {};

    for(const auto image: images) {
        descriptorSet.bind(bindIndex++, *image);

        // Associated sampler
        auto& samplerInfo = samplerInfos[samplerIndex++];
        samplerInfo.setImageLayout(image->initialLayout());
        samplerInfo.setImageView(image->fullImageView());
        samplerInfo.setSampler(*linearClampedSampler);

        auto write = descriptorSet.writeFromBinding(bindIndex++);
        write.setPImageInfo(&samplerInfo);

        descriptorSet.bind(write);
    }

    descriptorSet.update();
}

UniqueComputePass ComputeSystem::createComputePass(string_view name)
{
    return UniqueComputePass{new ComputePass{name}};
}

void ComputeSystem::bindDescriptorSet(vk::CommandBuffer& cmd)
{
    cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *computePipelineLayout, 0, descriptorSet.set(), {});
}

} // namespace raygun::compute
