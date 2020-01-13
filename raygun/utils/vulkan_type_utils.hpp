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

namespace raygun::vulkan {

template<typename T>
constexpr vk::DebugReportObjectTypeEXT getDebugReportObjectTypeEXT()
{
    if constexpr(false) {
    }

#define VULKAN_TYPE(_type) \
    else if constexpr(std::is_same_v<T, vk::_type>) { return vk::DebugReportObjectTypeEXT::e##_type; }
#include "raygun/utils/vulkan_types.def"

    else {
        return vk::DebugReportObjectTypeEXT::eUnknown;
    }
}

template<typename T>
struct is_vulkan_type {
    static constexpr bool value = getDebugReportObjectTypeEXT<T>() != vk::DebugReportObjectTypeEXT::eUnknown;
};

template<typename T>
constexpr bool is_vulkan_type_v = is_vulkan_type<T>::value;

} // namespace raygun::vulkan
