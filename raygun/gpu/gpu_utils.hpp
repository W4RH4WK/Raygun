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

#include "raygun/assert.hpp"
#include "raygun/utils/io_utils.hpp"

namespace raygun::gpu {

/// C API calls yield objects that should be wrapped in a UniqueHandler. This
/// function initializes the corresponding deleter for you.
template<typename T, typename D>
vk::UniqueHandle<T, vk::DispatchLoaderDynamic> wrapUnique(const T& value, const D& destroyer)
{
    const auto deleter = vk::ObjectDestroy<D, vk::DispatchLoaderDynamic>{destroyer};
    return vk::UniqueHandle<T, vk::DispatchLoaderDynamic>{value, deleter};
}

/// While you commonly store vectors of UniqueHandles, most C++ API calls take a
/// vector of raw handles since no transfer of ownership occurs. This utility
/// function does the necessary unwrapping.
template<typename T>
std::vector<T> unwrapUniques(std::vector<vk::UniqueHandle<T, vk::DispatchLoaderDynamic>>& uniques)
{
    const auto deref = [](auto& unique) { return *unique; };

    std::vector<T> ret;
    ret.reserve(uniques.size());
    std::transform(uniques.begin(), uniques.end(), std::back_inserter(ret), deref);
    return ret;
}

/// Vulkan requires manual selection of memory types, depending on the
/// requirements and what is provided by the hardware.
uint32_t selectMemoryType(const vk::PhysicalDevice& physicalDevice, uint32_t supportedMemoryTypes, vk::MemoryPropertyFlags additionalRequirements);

vk::ImageSubresourceRange mipImageSubresourceRange(uint32_t mipIndex, uint32_t mipCount = 1);
vk::ImageSubresourceRange defaultImageSubresourceRange();

} // namespace raygun::gpu
