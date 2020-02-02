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

#include "raygun/gpu/gpu_buffer.hpp"
#include "raygun/gpu/image.hpp"
#include "raygun/render/acceleration_structure.hpp"
#include "raygun/vulkan_context.hpp"

namespace raygun::gpu {

/// This abstracts Vulkan's descriptor set.
///
/// It uses a dedicated pool and layout. Bindings are registered before pool,
/// layout, and set are generated.
class DescriptorSet {
  public:
    DescriptorSet();

    /// Registers a new binding. Can only be used before the descriptor set has
    /// been generated.
    void addBinding(uint32_t binding, uint32_t count, vk::DescriptorType type, vk::ShaderStageFlags stage);

    /// Generates pool, layout, and set with the registered bindings.
    void generate();

    /// Record a binding request to be executed on the next `update` call.
    void bind(uint32_t binding, const Buffer& buffer);
    void bind(uint32_t binding, const Image& image);
    void bind(uint32_t binding, const render::TopLevelAS& accelerationStructure);
    void bind(vk::WriteDescriptorSet write);

    /// Executes all pending binding requests.
    void update();

    vk::WriteDescriptorSet writeFromBinding(uint32_t binding);

    const vk::DescriptorPool& pool() const { return *m_pool; }

    const vk::DescriptorSetLayout& layout() const { return *m_layout; }

    const vk::DescriptorSet& set() const { return m_set; }

  private:
    VulkanContext& vc;

    std::unordered_map<uint32_t, vk::DescriptorSetLayoutBinding> m_bindings;

    vk::UniqueDescriptorPool m_pool;
    vk::UniqueDescriptorPool generatePool() const;

    vk::UniqueDescriptorSetLayout m_layout;
    vk::UniqueDescriptorSetLayout generateLayout() const;

    vk::DescriptorSet m_set;
    vk::DescriptorSet generateSet() const;

    std::vector<vk::WriteDescriptorSet> m_pendingWrites;
};

} // namespace raygun::gpu
