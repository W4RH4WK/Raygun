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

#include "raygun/gpu/image.hpp"

#include "raygun/gpu/gpu_utils.hpp"
#include "raygun/raygun.hpp"

namespace raygun::gpu {

Image::Image(vk::Extent2D extent, vk::Format format, uint32_t numMipLayers, vk::SampleCountFlagBits samples, vk::ImageLayout layout)
    : m_extent(extent)
    , m_format(format)
    , m_numMips(numMipLayers)
    , m_samples(samples)
    , m_initialLayout(layout)
    , vc(RG().vc())
{
    RAYGUN_ASSERT(m_numMips > 0);

    setupImage();

    setupImageMemory();

    setupImageViews();

    // barrier
    {
        auto cmd = vc.graphicsQueue->createCommandBuffer();
        vc.setObjectName(*cmd, "Image Constructor");

        auto fence = vc.device->createFenceUnique({});
        vc.setObjectName(*fence, "Image Constructor");

        vk::CommandBufferBeginInfo beginInfo;
        beginInfo.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
        cmd->begin(beginInfo);

        vk::ImageMemoryBarrier barr;
        barr.setImage(*m_image);
        barr.setOldLayout(vk::ImageLayout::eUndefined);
        barr.setNewLayout(layout);
        barr.setSubresourceRange(mipImageSubresourceRange(0, m_numMips));

        cmd->pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eAllCommands, vk::DependencyFlagBits::eByRegion, {}, {}, barr);

        cmd->end();
        vc.graphicsQueue->submit(*cmd, *fence);
        vc.waitForFence(*fence);
    }

    {
        m_descriptorInfo.resize(m_numMips);
        for(uint32_t i = 0; i < m_numMips; ++i) {
            m_descriptorInfo[i].setImageLayout(m_initialLayout);
            m_descriptorInfo[i].setImageView(imageView(i));
        }
    }
}

void Image::setName(string_view name)
{
    vc.setObjectName(*m_image, name);
    vc.setObjectName(*m_fullImageView, name);
    vc.setObjectName(*m_imageMemory, name);

    for(const auto& view: m_imageViews) {
        vc.setObjectName(*view, name);
    }
}

void Image::setupImage()
{
    vk::ImageCreateInfo info;
    info.setArrayLayers(1);
    info.setExtent({m_extent.width, m_extent.height, 1});
    info.setFormat(m_format);
    info.setImageType(vk::ImageType::e2D);
    info.setInitialLayout(vk::ImageLayout::eUndefined);
    info.setMipLevels(m_numMips);
    info.setQueueFamilyIndexCount(0);
    info.setSamples(m_samples);
    info.setSharingMode(vk::SharingMode::eExclusive);
    info.setTiling(vk::ImageTiling::eOptimal);
    info.setUsage(vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eSampled);

    m_image = vc.device->createImageUnique(info);
}

void Image::setupImageViews()
{
    vk::ImageViewCreateInfo fullInfo;
    fullInfo.setFormat(m_format);
    fullInfo.setImage(*m_image);
    fullInfo.setSubresourceRange(mipImageSubresourceRange(0, m_numMips));
    fullInfo.setViewType(vk::ImageViewType::e2D);
    m_fullImageView = vc.device->createImageViewUnique(fullInfo);

    for(uint32_t i = 0; i < m_numMips; ++i) {
        vk::ImageViewCreateInfo info;
        info.setFormat(m_format);
        info.setImage(*m_image);
        info.setSubresourceRange(mipImageSubresourceRange(i));
        info.setViewType(vk::ImageViewType::e2D);
        m_imageViews.emplace_back(vc.device->createImageViewUnique(info));
    }
}

void Image::setupImageMemory()
{
    const auto requirements = vc.device->getImageMemoryRequirements(*m_image);

    const auto memoryType = selectMemoryType(vc.physicalDevice, requirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);

    vk::MemoryAllocateInfo info;
    info.setAllocationSize(requirements.size);
    info.setMemoryTypeIndex(memoryType);

    m_imageMemory = vc.device->allocateMemoryUnique(info);

    vc.device->bindImageMemory(*m_image, *m_imageMemory, 0);
}

} // namespace raygun::gpu
