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

class Image {
  public:
    Image(vk::Extent2D extent, vk::Format format = vk::Format::eR16G16B16A16Sfloat, uint32_t numMipLayers = 1,
          vk::SampleCountFlagBits samples = vk::SampleCountFlagBits::e1, vk::ImageLayout initialLayout = vk::ImageLayout::eGeneral);

    const vk::Extent2D& extent() const { return m_extent; }
    const vk::Format& format() const { return m_format; }
    uint32_t numMips() const { return m_numMips; }
    const vk::ImageLayout& initialLayout() const { return m_initialLayout; }

    const vk::Image& image() const { return *m_image; }
    const vk::ImageView& imageView(uint32_t mipLevel = 0) const { return *m_imageViews[mipLevel]; }
    const vk::ImageView& fullImageView() const { return *m_fullImageView; }

    const std::vector<vk::DescriptorImageInfo>& descriptorInfo() const { return m_descriptorInfo; }

  private:
    vk::Extent2D m_extent;
    vk::Format m_format;
    uint32_t m_numMips;
    vk::SampleCountFlagBits m_samples;
    vk::ImageLayout m_initialLayout;

    vk::UniqueImage m_image;
    vk::UniqueImageView m_fullImageView;
    std::vector<vk::UniqueImageView> m_imageViews;
    vk::UniqueDeviceMemory m_imageMemory;

    std::vector<vk::DescriptorImageInfo> m_descriptorInfo;

    VulkanContext& vc;

    void setupImage();
    void setupImageViews();
    void setupImageMemory();
};

using UniqueImage = std::unique_ptr<Image>;

} // namespace raygun::gpu
