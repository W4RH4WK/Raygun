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

#include "raygun/gpu/gpu_utils.hpp"

namespace raygun::gpu {

uint32_t selectMemoryType(const vk::PhysicalDevice& physicalDevice, uint32_t supportedMemoryTypes, vk::MemoryPropertyFlags additionalRequirements)
{
    const auto memoryProperties = physicalDevice.getMemoryProperties();

    for(uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i) {
        const auto& memoryType = memoryProperties.memoryTypes[i];

        if(((supportedMemoryTypes & 1) == 1) && (memoryType.propertyFlags & additionalRequirements) == additionalRequirements) return i;

        supportedMemoryTypes >>= 1;
    }

    RAYGUN_ASSERT(false);
    return UINT32_MAX;
}

vk::ImageSubresourceRange mipImageSubresourceRange(uint32_t mipIndex, uint32_t mipCount)
{
    vk::ImageSubresourceRange range;
    range.setAspectMask(vk::ImageAspectFlagBits::eColor);
    range.setLayerCount(1);
    range.setBaseMipLevel(mipIndex);
    range.setLevelCount(mipCount);

    return range;
}

vk::ImageSubresourceRange defaultImageSubresourceRange()
{
    return mipImageSubresourceRange(0);
}

vk::ImageSubresourceLayers defaultImageSubresourceLayers()
{
    vk::ImageSubresourceLayers subresourceLayers;
    subresourceLayers.setMipLevel(0);
    subresourceLayers.setLayerCount(1);
    subresourceLayers.setAspectMask(vk::ImageAspectFlagBits::eColor);

    return subresourceLayers;
}

} // namespace raygun::gpu
