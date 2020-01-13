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

#version 460
#extension GL_NV_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_GOOGLE_include_directive : enable

#include "payload.h"
#include "raytracer_bindings.h"

layout(binding = RAYGUN_RAYTRACER_BINDING_UNIFORM_BUFFER, set = 0) uniform UniformBufferObject{
#include "uniform_buffer_object.def"
} ubo;

// Based on Simple Sky Shader by robobo1221, see
// https://www.shadertoy.com/view/MsVSWt

vec3 skyMix(vec3 sunTone, vec3 skyTone, vec3 scatterTone, float scatterFactor, float powFactor)
{
    vec3 rayDir = normalize(gl_WorldRayDirectionNV);
    float y = abs(gl_WorldRayDirectionNV.y + 1.5) / 3.0;

    float sun = 1.0 - distance(rayDir, normalize(-ubo.lightDir));
    sun = clamp(sun, 0.0, 2.0);

    float glow = sun;
    glow = clamp(glow, 0.0, 1.0);

    sun = pow(sun, powFactor);
    sun *= 1000.0;
    sun = clamp(sun, 0.0, 16.0);

    glow = pow(glow, 6.0) * 1.0;
    glow = pow(glow, y);
    glow = clamp(glow, 0.0, 1.0);

    sun *= pow(dot(y, y), 1.0 / 1.65);

    glow *= pow(dot(y, y), 1.0 / 2.0);

    sun += glow;

    vec3 sunColor = sunTone * sun;

    float atmosphere = sqrt(1.0 - y);

    float scatter = pow(4 - ubo.lightDir.y, 1.0 / 15.0);
    scatter = 1.0 - clamp(scatter, 0.8, 1.0);

    vec3 scatterColor = mix(vec3(1.0), scatterTone * 1.5, scatter);
    vec3 skyScatter = mix(skyTone, vec3(scatterColor), atmosphere / scatterFactor);

    return sunColor + skyScatter;
}

void main()
{
    vec3 res = skyMix(vec3(1.0, 0.6, 0.05), vec3(0.2, 0.4, 0.8), vec3(1.0, 0.3, 0.0), 1.3, 80);

    payload.hitValue = res;
    payload.roughValue = vec4(res, 0);
    payload.depth = 10000.f;
}
