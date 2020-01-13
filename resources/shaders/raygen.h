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

#include "raytracer_bindings.h"

layout(binding = RAYGUN_RAYTRACER_BINDING_ACCELERATION_STRUCTURE, set = 0) uniform accelerationStructureNV topLevelAS;
layout(binding = RAYGUN_RAYTRACER_BINDING_OUTPUT_IMAGE, set = 0, rgba16f) restrict uniform image2D image;
layout(binding = RAYGUN_RAYTRACER_BINDING_ROUGH_IMAGE, set = 0, rgba16f) restrict uniform image2D roughImage;
layout(binding = RAYGUN_RAYTRACER_BINDING_NORMAL_IMAGE, set = 0, rgba16f) restrict uniform image2D normalImage;

layout(binding = RAYGUN_RAYTRACER_BINDING_UNIFORM_BUFFER, set = 0) restrict uniform UniformBufferObject{
#include "uniform_buffer_object.def"
} ubo;

#define RAYGEN
#include "payload.h"

const vec2 vAAOffsets[9][8] = {
    // clang-format off
    { vec2(0.00, 0.00), vec2(0.00, 0.00), vec2(0.00, 0.00), vec2(0.00, 0.00),
      vec2(0.00, 0.00), vec2(0.00, 0.00), vec2(0.00, 0.00), vec2(0.00, 0.00),
    },
    { vec2(0.00, 0.00), vec2(0.00, 0.00), vec2(0.00, 0.00), vec2(0.00, 0.00),
      vec2(0.00, 0.00), vec2(0.00, 0.00), vec2(0.00, 0.00), vec2(0.00, 0.00),
    },
    { vec2(0.25, 0.25), vec2(-0.25, -0.25), vec2(0.00, 0.00), vec2(0.00, 0.00),
      vec2(0.00, 0.00), vec2(0.00, 0.00), vec2(0.00, 0.00), vec2(0.00, 0.00),
    },
    { vec2(-0.125, -0.375), vec2(0.375, -0.125), vec2(-0.375, 0.125), vec2(0.125, 0.375),
      vec2(0.00, 0.00), vec2(0.00, 0.00), vec2(0.00, 0.00), vec2(0.00, 0.00),
    },
    { vec2(-0.125, -0.375), vec2(0.375, -0.125), vec2(-0.375, 0.125), vec2(0.125, 0.375),
      vec2(0.00, 0.00), vec2(0.00, 0.00), vec2(0.00, 0.00), vec2(0.00, 0.00),
    },
    { vec2(0.0625, -0.1875), vec2(-0.0625, 0.1875), vec2(0.3125, 0.0625), vec2(-0.1875, -0.3125),
      vec2(-0.3125, 0.3125), vec2(-0.4375, -0.0625), vec2(0.1875, 0.4375), vec2(0.4375, -0.4375),
    },
    { vec2(0.0625, -0.1875), vec2(-0.0625, 0.1875), vec2(0.3125, 0.0625), vec2(-0.1875, -0.3125),
      vec2(-0.3125, 0.3125), vec2(-0.4375, -0.0625), vec2(0.1875, 0.4375), vec2(0.4375, -0.4375),
    },
    { vec2(0.0625, -0.1875), vec2(-0.0625, 0.1875), vec2(0.3125, 0.0625), vec2(-0.1875, -0.3125),
      vec2(-0.3125, 0.3125), vec2(-0.4375, -0.0625), vec2(0.1875, 0.4375), vec2(0.4375, -0.4375),
    },
    { vec2(0.0625, -0.1875), vec2(-0.0625, 0.1875), vec2(0.3125, 0.0625), vec2(-0.1875, -0.3125),
      vec2(-0.3125, 0.3125), vec2(-0.4375, -0.0625), vec2(0.1875, 0.4375), vec2(0.4375, -0.4375),
    },
    // clang-format on
};

mat3x4 traceRay(int numSamples)
{
    vec3 color = vec3(0);
    vec3 normal = vec3(0);
    vec4 roughValue = vec4(0);
    float reflectContrib = 0;
    float depth = 0;
    // float prevRough = 0;

    const vec4 origin = ubo.viewInverse * vec4(0, 0, 0, 1);
    const uint rayFlags = gl_RayFlagsOpaqueNV;
    const uint cullMask = 0xff;
    const float tmin = 0.001;
    const float tmax = 10000.0;

    for(int i = 0; i < numSamples; ++i) {
        const vec2 pixelCenter = vec2(gl_LaunchIDNV.xy) + vec2(0.5) + vAAOffsets[min(numSamples, 8)][i % 8];
        const vec2 inUV = pixelCenter / vec2(gl_LaunchSizeNV.xy);
        const vec2 d = inUV * 2.0 - 1.0;

        const vec4 target = ubo.projInverse * vec4(d.x, d.y, 1, 1);
        const vec4 direction = ubo.viewInverse * vec4(normalize(target.xyz), 0);

        payload.hitValue = vec3(0);
        payload.normal = vec3(0);
        payload.roughValue = vec4(0);
        payload.depth = 0;
        payload.refDepth = 0;
        payload.curIOR = 1.0f;
        payload.rayType = RT_GENERIC;
        payload.recDepth = 0;
        payload.reflectContribution = 0;

        traceNV(topLevelAS, rayFlags, cullMask, 0 /*sbtRecordOffset*/, 0 /*sbtRecordStride*/, 0 /*missIndex*/, origin.xyz, tmin, direction.xyz, tmax,
                0 /*payload*/);

        color += payload.hitValue;
        normal += payload.normal;
        payload.roughValue.a = payload.roughValue.a;
        // if(i>0) payload.roughValue.a = min(prevRough, payload.roughValue.a);
        roughValue += payload.roughValue;
        reflectContrib += payload.reflectContribution;
        depth += payload.depth;
    }

    return mat3x4(vec4(color, reflectContrib), vec4(normal, log(depth) * 0.25), roughValue) / float(numSamples);
}
