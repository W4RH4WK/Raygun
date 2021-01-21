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

#pragma once

#define RAYGUN_ARRAY_COUNT(_x) ((sizeof(_x) / sizeof(0 [_x])) / ((size_t)(!(sizeof(_x) % sizeof(0 [_x])))))

namespace raygun::utils {

template<typename T, size_t S>
static inline T mean(const std::array<T, S>& arr)
{
    auto sum = std::accumulate(arr.begin(), arr.end(), T{}, [&](auto& sum, const auto& e) { return sum + e; });
    return sum / arr.size();
}

} // namespace raygun::utils
