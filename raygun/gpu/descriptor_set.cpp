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

#include "raygun/gpu/descriptor_set.hpp"

#include "raygun/assert.hpp"
#include "raygun/raygun.hpp"

namespace raygun::gpu {

DescriptorSet::DescriptorSet() : vc(RG().vc()) {}

void DescriptorSet::addBinding(uint32_t binding, uint32_t count, vk::DescriptorType type, vk::ShaderStageFlags stage)
{
    if(m_set) {
        RAYGUN_FATAL("DescriptorSet already generated, cannot add more bindings");
    }

    if(m_bindings.find(binding) != m_bindings.end()) {
        RAYGUN_WARN("Rebinding descriptor {}", binding);
    }

    vk::DescriptorSetLayoutBinding b = {};
    b.setBinding(binding);
    b.setDescriptorCount(count);
    b.setDescriptorType(type);
    b.setStageFlags(stage);

    m_bindings[binding] = b;
}

void DescriptorSet::generate()
{
    m_pool = generatePool();

    m_layout = generateLayout();

    m_set = generateSet();
}

void DescriptorSet::bind(uint32_t binding, const Buffer& buffer)
{
    auto write = writeFromBinding(binding);
    write.setPBufferInfo(&buffer.descriptorInfo());
    RAYGUN_ASSERT(write.descriptorCount == 1);

    m_pendingWrites.push_back(write);
}

void DescriptorSet::bind(uint32_t binding, const Image& image)
{
    auto write = writeFromBinding(binding);
    write.setPImageInfo(image.descriptorInfo().data());
    RAYGUN_ASSERT(write.descriptorCount == 1);

    m_pendingWrites.push_back(write);
}

void DescriptorSet::bind(uint32_t binding, const render::TopLevelAS& accelerationStructure)
{
    auto write = writeFromBinding(binding);
    write.setPNext(&accelerationStructure.descriptorInfo());
    RAYGUN_ASSERT(write.descriptorCount == 1);

    m_pendingWrites.push_back(write);
}

void DescriptorSet::bind(vk::WriteDescriptorSet write)
{
    m_pendingWrites.push_back(write);
}

void DescriptorSet::update()
{
    vc.device->updateDescriptorSets(m_pendingWrites, {});

    m_pendingWrites.clear();
}

vk::WriteDescriptorSet DescriptorSet::writeFromBinding(uint32_t index)
{
    RAYGUN_ASSERT(m_set);

    const auto& binding = m_bindings.at(index);

    vk::WriteDescriptorSet write = {};
    write.setDstSet(m_set);
    write.setDstBinding(index);
    write.setDescriptorType(binding.descriptorType);
    write.setDescriptorCount(binding.descriptorCount);

    return write;
}

vk::UniqueDescriptorPool DescriptorSet::generatePool() const
{
    constexpr auto toSize = [](auto it) { return vk::DescriptorPoolSize{it.second.descriptorType, it.second.descriptorCount}; };

    std::vector<vk::DescriptorPoolSize> sizes(m_bindings.size());
    std::transform(m_bindings.begin(), m_bindings.end(), sizes.begin(), toSize);

    vk::DescriptorPoolCreateInfo info = {};
    info.setPoolSizeCount((uint32_t)sizes.size());
    info.setPPoolSizes(sizes.data());
    info.setMaxSets(1);

    return vc.device->createDescriptorPoolUnique(info);
}

vk::UniqueDescriptorSetLayout DescriptorSet::generateLayout() const
{
    std::vector<vk::DescriptorSetLayoutBinding> bindings(m_bindings.size());
    std::transform(m_bindings.begin(), m_bindings.end(), bindings.begin(), [](auto it) { return it.second; });

    vk::DescriptorSetLayoutCreateInfo info = {};
    info.setBindingCount((uint32_t)bindings.size());
    info.setPBindings(bindings.data());

    return vc.device->createDescriptorSetLayoutUnique(info);
}

vk::DescriptorSet DescriptorSet::generateSet() const
{
    RAYGUN_ASSERT(m_pool);
    RAYGUN_ASSERT(m_layout);

    vk::DescriptorSetAllocateInfo info = {};
    info.setDescriptorPool(*m_pool);
    info.setDescriptorSetCount(1);
    info.setPSetLayouts(&*m_layout);

    return vc.device->allocateDescriptorSets(info).at(0);
}

} // namespace raygun::gpu
