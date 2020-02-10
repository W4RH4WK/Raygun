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

#include "raygun/render/swapchain.hpp"

#include "raygun/assert.hpp"
#include "raygun/raygun.hpp"

namespace raygun::render {

Swapchain::Swapchain(RenderSystem& renderSystem) : vc(RG().vc()), renderSystem(renderSystem)
{
    m_format = vc.surfaceFormat;

    setupSwapchain();

    setupImages();

    setupImageViews();

    setupFramebuffers();
}

uint32_t Swapchain::nextImageIndex(vk::Semaphore imageAcquiredSemaphore)
{
    return vc.device->acquireNextImageKHR(*m_swapchain, UINT64_MAX, imageAcquiredSemaphore, nullptr).value;
}

void Swapchain::setupSwapchain()
{
    const auto capabilities = vc.physicalDevice.getSurfaceCapabilitiesKHR(*vc.surface);

    const uint32_t minImageCount = 2;
    RAYGUN_ASSERT(capabilities.minImageCount <= minImageCount);

    const auto imageExtent = vc.windowSize;
    RAYGUN_ASSERT(capabilities.minImageExtent.width <= imageExtent.width);
    RAYGUN_ASSERT(imageExtent.width <= capabilities.maxImageExtent.width);
    RAYGUN_ASSERT(capabilities.minImageExtent.height <= imageExtent.height);
    RAYGUN_ASSERT(imageExtent.height <= capabilities.maxImageExtent.height);

    auto preTransform = capabilities.currentTransform;
    if(capabilities.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity) {
        preTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity;
    }

    auto compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    {
        const auto wantedFlags = {
            vk::CompositeAlphaFlagBitsKHR::eOpaque,
            vk::CompositeAlphaFlagBitsKHR::ePreMultiplied,
            vk::CompositeAlphaFlagBitsKHR::ePostMultiplied,
            vk::CompositeAlphaFlagBitsKHR::eInherit,
        };

        auto it = std::find_if(wantedFlags.begin(), wantedFlags.end(), [&](const auto& flag) { return capabilities.supportedCompositeAlpha & flag; });
        if(it != wantedFlags.end()) {
            compositeAlpha = *it;
        }
    }

    const uint32_t queueFamilyIndices[] = {vc.graphicsQueueFamilyIndex, vc.presentQueueFamilyIndex};

    auto presentMode = vk::PresentModeKHR::eFifo;
    switch(RG().config().presentMode) {
    case Config::PresentMode::Immediate:
        presentMode = vk::PresentModeKHR::eImmediate;
        break;
    case Config::PresentMode::Mailbox:
        presentMode = vk::PresentModeKHR::eMailbox;
        break;
    case Config::PresentMode::Fifo:
        presentMode = vk::PresentModeKHR::eFifo;
        break;
    case Config::PresentMode::FifoRelaxed:
        presentMode = vk::PresentModeKHR::eFifoRelaxed;
        break;
    }

    const auto presentModes = vc.physicalDevice.getSurfacePresentModesKHR(*vc.surface);
    if(std::find(presentModes.begin(), presentModes.end(), presentMode) == presentModes.end()) {
        RAYGUN_WARN("Unsupported present mode: {}", to_string(presentMode));
        presentMode = vk::PresentModeKHR::eFifo;
    }

    vk::SwapchainCreateInfoKHR info = {};
    info.setSurface(*vc.surface);
    info.setMinImageCount(minImageCount);
    info.setImageFormat(m_format);
    info.setImageExtent(imageExtent);
    info.setPreTransform(preTransform);
    info.setCompositeAlpha(compositeAlpha);
    info.setImageArrayLayers(1);
    info.setPresentMode(presentMode);
    info.setClipped(true);
    info.setImageColorSpace(vk::ColorSpaceKHR::eSrgbNonlinear);
    info.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst);
    info.setPQueueFamilyIndices(queueFamilyIndices);

    // Graphics and presentation queue could be different, in which case we
    // need to enable image sharing.
    if(vc.graphicsQueueFamilyIndex == vc.presentQueueFamilyIndex) {
        info.setQueueFamilyIndexCount(1);
        info.setImageSharingMode(vk::SharingMode::eExclusive);
    }
    else {
        info.setQueueFamilyIndexCount(2);
        info.setImageSharingMode(vk::SharingMode::eConcurrent);
    }

    if(m_swapchain) {
        info.setOldSwapchain(*m_swapchain);
    }

    m_swapchain = vc.device->createSwapchainKHRUnique(info);
}

void Swapchain::setupImages()
{
    m_images = vc.device->getSwapchainImagesKHR(*m_swapchain);
    for(const auto& image: m_images) {
        vc.setObjectName(image, "Swapchain");
    }
}

void Swapchain::setupImageViews()
{
    m_imageViews.clear();

    for(const auto& image: m_images) {
        vk::ImageSubresourceRange subresourceRange = {};
        subresourceRange.setAspectMask(vk::ImageAspectFlagBits::eColor);
        subresourceRange.setLevelCount(1);
        subresourceRange.setLayerCount(1);

        vk::ImageViewCreateInfo info = {};
        info.setImage(image);
        info.setViewType(vk::ImageViewType::e2D);
        info.setFormat(m_format);
        info.setSubresourceRange(subresourceRange);

        info.setComponents({
            vk::ComponentSwizzle::eR,
            vk::ComponentSwizzle::eG,
            vk::ComponentSwizzle::eB,
            vk::ComponentSwizzle::eA,
        });

        auto imageView = vc.device->createImageViewUnique(info);
        vc.setObjectName(*imageView, "Swapchain");

        m_imageViews.push_back(std::move(imageView));
    }
}

void Swapchain::setupFramebuffers()
{
    m_framebuffers.clear();

    for(const auto& imageView: m_imageViews) {
        vk::FramebufferCreateInfo info = {};
        info.setRenderPass(renderSystem.renderPass());
        info.setAttachmentCount(1);
        info.setPAttachments(&*imageView);
        info.setWidth(vc.windowSize.width);
        info.setHeight(vc.windowSize.height);
        info.setLayers(1);

        auto frameBuffer = vc.device->createFramebufferUnique(info);
        vc.setObjectName(*frameBuffer, "Swapchain");

        m_framebuffers.push_back(std::move(frameBuffer));
    }
}

} // namespace raygun::render
