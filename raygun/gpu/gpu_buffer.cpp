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

#include "raygun/gpu/gpu_buffer.hpp"

#include "raygun/gpu/gpu_utils.hpp"
#include "raygun/logging.hpp"
#include "raygun/raygun.hpp"

namespace raygun::gpu {

Buffer::Buffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags memoryType) : vc(RG().vc())
{
    RAYGUN_TRACE("Creating buffer: {} bytes", size);

    if(size == 0) {
        RAYGUN_DEBUG("Buffers of size 0 byte not supported, setting size to 1 byte");
        size = 1;
    }

    vk::BufferCreateInfo createInfo = {};
    createInfo.setUsage(usage);
    createInfo.setSize(size);
    createInfo.setSharingMode(vk::SharingMode::eExclusive);

    m_buffer = vc.device->createBufferUnique(createInfo);

    m_info.setBuffer(*m_buffer);
    m_info.setRange(size);

    alloc(usage, memoryType);

    vc.device->bindBufferMemory(*m_buffer, *m_memory, 0);
}

Buffer::~Buffer()
{
    unmap();
}

void* Buffer::map()
{
    if(!m_mappedMemory) {
        m_mappedMemory = vc.device->mapMemory(*m_memory, 0, m_allocInfo.allocationSize);
    }

    return m_mappedMemory;
}

void Buffer::unmap()
{
    if(m_mappedMemory) {
        vc.device->unmapMemory(*m_memory);
        m_mappedMemory = nullptr;
    }
}

vk::DeviceAddress Buffer::address() const
{
    return vc.device->getBufferAddress({*m_buffer});
}

void Buffer::setName(string_view name)
{
    vc.setObjectName(*m_buffer, name);
    vc.setObjectName(*m_memory, name);
}

void Buffer::alloc(vk::BufferUsageFlags usage, vk::MemoryPropertyFlags memoryTypeFlags)
{
    const auto requirements = vc.device->getBufferMemoryRequirements(*m_buffer);

    const auto memoryType = selectMemoryType(vc.physicalDevice, requirements.memoryTypeBits, memoryTypeFlags);

    vk::MemoryAllocateFlagsInfo allocFlagsInfo = {};

    if(usage & vk::BufferUsageFlagBits::eShaderDeviceAddress) {
        allocFlagsInfo.setFlags(vk::MemoryAllocateFlagBits::eDeviceAddress);
    }

    m_allocInfo.setAllocationSize(requirements.size);
    m_allocInfo.setMemoryTypeIndex(memoryType);
    m_allocInfo.setPNext(&allocFlagsInfo);

    m_memory = vc.device->allocateMemoryUnique(m_allocInfo);

    // allocFlagsInfo is no longer available after alloc call.
    m_allocInfo.setPNext(nullptr);
}

} // namespace raygun::gpu
