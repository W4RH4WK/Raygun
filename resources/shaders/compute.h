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

#include "compute_shader_shared.def"
layout(local_size_x = COMPUTE_WG_X_SIZE, local_size_y = COMPUTE_WG_Y_SIZE, local_size_z = COMPUTE_WG_Z_SIZE) in;

layout(binding = 0, set = 0) uniform UniformBufferObject{
#include "uniform_buffer_object.def"
} ubo;

layout(binding = 1, set = 0, rgba16f) restrict uniform image2D finalImage;
layout(binding = 2, set = 0) uniform sampler2D finalSampler;
layout(binding = 3, set = 0, rgba16f) restrict uniform image2D baseImage;
layout(binding = 4, set = 0) uniform sampler2D baseSampler;
layout(binding = 5, set = 0, rgba16f) restrict uniform image2D normalImage;
layout(binding = 6, set = 0) uniform sampler2D normalSampler;
layout(binding = 7, set = 0, rgba16f) restrict uniform image2D roughImage;
layout(binding = 8, set = 0) uniform sampler2D roughSampler;

layout(binding = 9, set = 0, rgba8) restrict uniform image2D roughTransitions;
layout(binding = 10, set = 0) uniform sampler2D roughTransitionsSampler;
layout(binding = 11, set = 0, rgba16f) restrict uniform image2D roughColorsA;
layout(binding = 12, set = 0) uniform sampler2D roughColorsASampler;
layout(binding = 13, set = 0, rgba16f) restrict uniform image2D roughColorsB;
layout(binding = 14, set = 0) uniform sampler2D roughColorsBSampler;
