// The MIT License (MIT)
//
// Copyright (c) 2019-2021 The Raygun Authors.
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

#include "raygun/gpu/gpu_queue.hpp"

namespace raygun::gpu {

Queue::Queue(const vk::Device& device, uint32_t familyIndex) : m_familyIndex(familyIndex), m_device(device)
{
    m_queue = device.getQueue(m_familyIndex, 0);

    setupCommandPool();
}

vk::UniqueCommandBuffer Queue::createCommandBuffer()
{
    vk::CommandBufferAllocateInfo info;
    info.setCommandPool(*m_commandPool);
    info.setLevel(vk::CommandBufferLevel::ePrimary);
    info.setCommandBufferCount(1);

    return std::move(m_device.allocateCommandBuffersUnique(info).at(0));
}

void Queue::submit(vk::ArrayProxy<const vk::CommandBuffer> cmds, vk::Fence fence, vk::ArrayProxy<const vk::Semaphore> signalSemaphores,
                   vk::ArrayProxy<const vk::Semaphore> waitSemaphores)
{
    vk::SubmitInfo info = {};
    info.setCommandBufferCount(cmds.size());
    info.setPCommandBuffers(cmds.data());
    info.setSignalSemaphoreCount(signalSemaphores.size());
    info.setPSignalSemaphores(signalSemaphores.data());
    info.setWaitSemaphoreCount(waitSemaphores.size());
    info.setPWaitSemaphores(waitSemaphores.data());

    submit(info, fence);
}

void Queue::submit(vk::ArrayProxy<const vk::SubmitInfo> infos, vk::Fence fence)
{
    m_queue.submit(infos, fence);
}

void Queue::setupCommandPool()
{
    vk::CommandPoolCreateInfo info = {};
    info.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
    info.setQueueFamilyIndex(m_familyIndex);

    m_commandPool = m_device.createCommandPoolUnique(info);
}

void Queue::waitIdle()
{
    m_queue.waitIdle();
}

} // namespace raygun::gpu
