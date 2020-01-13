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

namespace raygun::gpu {

/// Utility class wrapping Vulkan queues and related operations.
class Queue {
  public:
    Queue(const vk::Device& device, uint32_t familyIndex);

    uint32_t familyIndex() const { return m_familyIndex; }

    vk::Queue& queue() { return m_queue; }

    vk::CommandPool& commandPool() { return *m_commandPool; }

    vk::UniqueCommandBuffer createCommandBuffer();

    void submit(vk::ArrayProxy<const vk::CommandBuffer> cmds, vk::Fence fence = {}, vk::ArrayProxy<const vk::Semaphore> signalSemaphores = {},
                vk::ArrayProxy<const vk::Semaphore> waitSemaphores = {});
    void submit(vk::ArrayProxy<const vk::SubmitInfo> infos, vk::Fence fence = {});

    void waitIdle();

  private:
    uint32_t m_familyIndex;

    vk::Queue m_queue;

    vk::UniqueCommandPool m_commandPool;

    const vk::Device& m_device;

    void setupCommandPool();
};

using UniqueQueue = std::unique_ptr<Queue>;

} // namespace raygun::gpu
