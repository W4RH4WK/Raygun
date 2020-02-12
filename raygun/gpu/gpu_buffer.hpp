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

namespace raygun::gpu {

/// Utility class wrapping Vulkan buffers and related operations.
class Buffer {
  public:
    Buffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags memoryType);
    ~Buffer();

    operator vk::Buffer() const { return *m_buffer; }

    vk::DeviceSize size() const { return m_info.range; }

    const vk::DeviceMemory& memory() const { return *m_memory; }

    const vk::DescriptorBufferInfo& descriptorInfo() const { return m_info; }

    void* map();
    void unmap();

    void setName(string_view name);

  private:
    void alloc(const vk::MemoryPropertyFlags& memoryTypeFlags);

    vk::UniqueBuffer m_buffer;
    vk::UniqueDeviceMemory m_memory;

    void* m_mappedMemory = nullptr;

    vk::DescriptorBufferInfo m_info = {};
    vk::MemoryAllocateInfo m_allocInfo = {};

    VulkanContext& vc;
};

using UniqueBuffer = std::unique_ptr<Buffer>;

/// Reference into a buffer with offset and size.
struct BufferRef {
    vk::Buffer buffer = {};
    vk::DeviceSize offset = 0;
    vk::DeviceSize size = 0;
    vk::DeviceSize elementSize = 1;

    uint32_t elementCount() const { return (uint32_t)(size / elementSize); }
};

/// Copy the data from an std::vector to a dedicated GPU buffer.
template<typename T>
UniqueBuffer copyToBuffer(const std::vector<T>& data, vk::BufferUsageFlags usageFlags)
{
    constexpr auto memoryProperties = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;

    const auto bufferSize = data.size() * sizeof(data[0]);
    auto buffer = std::make_unique<Buffer>(bufferSize, usageFlags, memoryProperties);

    memcpy(buffer->map(), data.data(), bufferSize);
    buffer->unmap();

    return buffer;
}

} // namespace raygun::gpu
