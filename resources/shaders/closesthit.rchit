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

#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_GOOGLE_include_directive : enable

#include "payload.h"
#include "raytracer_bindings.h"

hitAttributeEXT vec3 attribs;
layout(binding = RAYGUN_RAYTRACER_BINDING_ACCELERATION_STRUCTURE, set = 0) uniform accelerationStructureEXT topLevelAS;

layout(binding = RAYGUN_RAYTRACER_BINDING_UNIFORM_BUFFER, set = 0) uniform UniformBufferObject{
#include "uniform_buffer_object.def"
} ubo;

struct Vertex {
#include "vertex.def"
};

layout(binding = RAYGUN_RAYTRACER_BINDING_VERTEX_BUFFER, set = 0) buffer Vertices
{
    Vertex v[];
}
vertices;

layout(binding = RAYGUN_RAYTRACER_BINDING_INDEX_BUFFER, set = 0) buffer Indices
{
    uint i[];
}
indices;

struct Material {
#include "gpu_material.def"
};

layout(binding = RAYGUN_RAYTRACER_BINDING_MATERIAL_BUFFER, set = 0) buffer Materials
{
    Material m[];
}
materials;

struct InstanceOffsetTableEntry {
#include "instance_offset_table.def"
};

layout(binding = RAYGUN_RAYTRACER_BINDING_INSTANCE_OFFSET_TABLE, set = 0) buffer InstanceOffsetTable
{
    InstanceOffsetTableEntry e[];
}
instanceOffsetTable;

void gridEffect(inout Material mat, vec3 pos)
{
    float aa = (payload.refDepth + gl_HitTEXT + 8) / 30;
    float aa2 = aa / 2.0;

    float minmod = min(abs(mod((pos.x + 1000) * 10 + aa2, 20) - aa2), abs(mod((pos.z + 1000) * 10 + aa2, 20) - aa2));
    if(minmod < aa2) {
        minmod -= aa2 - pow(aa, 2) / 3.0;
        minmod *= 3.0 / pow(aa, 2);
        mat.diffuse *= mix(aa / 10, 1.0, minmod);
        mat.specular *= mix(aa / 10, 1.0, minmod);
        mat.reflectivity *= mix(aa / 10, 1.0, minmod);
    }

    if(mod((pos.x + 1000) * 5, 20) < 10 && mod((pos.z + 1000) * 5, 20) < 10) {
        mat.reflectivity *= 1.5;
    }
}

void main()
{
    // Gather inputs (barycentrics, vertices and material)
    const vec3 barycentrics = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);

    uint indexBufferOffset = instanceOffsetTable.e[gl_InstanceCustomIndexEXT].indexBufferOffset;
    uint i0 = indices.i[indexBufferOffset + 3 * gl_PrimitiveID + 0];
    uint i1 = indices.i[indexBufferOffset + 3 * gl_PrimitiveID + 1];
    uint i2 = indices.i[indexBufferOffset + 3 * gl_PrimitiveID + 2];

    uint vertexBufferOffset = instanceOffsetTable.e[gl_InstanceCustomIndexEXT].vertexBufferOffset;
    Vertex v0 = vertices.v[vertexBufferOffset + i0];
    Vertex v1 = vertices.v[vertexBufferOffset + i1];
    Vertex v2 = vertices.v[vertexBufferOffset + i2];

    uint materialBufferOffset = instanceOffsetTable.e[gl_InstanceCustomIndexEXT].materialBufferOffset + v0.matIndex;
    Material mat = materials.m[materialBufferOffset];

    // Compute world space position
    float tmin = 0.01;
    float tmax = 1000.0;
    vec3 origin = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;

    // Transform normal into world space
    vec3 vn = v0.normal * barycentrics.x + v1.normal * barycentrics.y + v2.normal * barycentrics.z;
    mat3 objToWorldNoTranslation = mat3(gl_ObjectToWorldEXT);
    vec3 vnInWorldSpace = normalize(objToWorldNoTranslation * vn);

    // Material effects
    if(mat.effectId == 1) gridEffect(mat, origin);

    // Check if we are a shadow tracing ray, and if so, handle appropriately
    if(payload.rayType == RT_SHADOW_INTERNAL) {
        // larger value -> more material contribution to shadow
        float thicknessModulation = clamp(gl_HitTEXT * (1 - mat.transparency) * 10, 0, 1);
        vec3 shadowCol = payload.hitValue - mix(vec3(0), normalize(1.1 - mat.diffuse) + 0.1, thicknessModulation);

        if(payload.recDepth < ubo.maxRecursions) {
            payload.rayType = RT_SHADOW_TRACE;
            payload.recDepth++;
            traceRayEXT(topLevelAS, gl_RayFlagsOpaqueEXT, 0xFF, 0, 0, 0 /* missIndex */, origin, tmin, gl_WorldRayDirectionEXT, tmax, 0 /* payload location */);
            payload.recDepth--;

            if(payload.depth < 1000) {
                payload.hitValue *= shadowCol;
            }
            else {
                float eta = mat.ior / 1.0;
                vec3 dir = refract(gl_WorldRayDirectionEXT, vnInWorldSpace, eta);
                float dot_product = pow(dot(ubo.lightDir, dir), 5) + 0.75;
                shadowCol *= dot_product;
                traceRayEXT(topLevelAS, gl_RayFlagsOpaqueEXT, 0x0, 0, 0, 0 /* missIndex */, origin, tmin, -dir, tmax, 0 /* payload location */);
                payload.hitValue = shadowCol + .1 * payload.hitValue;
            }
        }
        else {
            payload.hitValue = shadowCol * vec3(0.4);
        }
        return;
    }
    if(payload.rayType == RT_SHADOW_TRACE) {
        if(mat.transparency > 0.0f) {
            if(payload.recDepth < ubo.maxRecursions) {
                payload.rayType = RT_SHADOW_INTERNAL;
                payload.recDepth++;
                traceRayEXT(topLevelAS, gl_RayFlagsOpaqueEXT, 0xFF, 0, 0, 1 /* missIndex */, origin, tmin, gl_WorldRayDirectionEXT, tmax, 0 /* payload location */);
                payload.recDepth--;
            }
        }
        else {
            payload.hitValue *= mix(vec3(0.4), vec3(0.8), clamp(log(gl_HitTEXT) / 8, 0, 1));
        }
        return;
    }

    // Correctly handle backfacing triangle illumination
    // (not required if there are no double-sided triangles)
    bool frontFacing = dot(-gl_WorldRayDirectionEXT, vnInWorldSpace) > 0;
    if(!frontFacing) vnInWorldSpace = normalize(-vnInWorldSpace);

    // Compute diffuse and specular lit color
    float dot_product = max(dot(-ubo.lightDir, vnInWorldSpace), 0.2);
    vec3 baseColor = dot_product * mat.diffuse;
    // vec3 specularColor = vec3(0.0, 0.0, 0.0);
    // if (dot(-ubo.lightDir, vnInWorldSpace) > 0.0) { // light source on the right side?
    //     const vec3 lightSpecular = vec3(1, 1, 1);         // TODO configurable?
    //     specularColor = lightSpecular * vec3(mat.specular) *
    //                     pow(max(0.0, dot(reflect(ubo.lightDir, vnInWorldSpace), normalize(-gl_WorldRayDirectionEXT))),
    //                         mat.reflectivity*100);
    // }
    // baseColor += specularColor;

    // Shadow computation - assume shadowed when not facing light
    // (0.07 instead of 0 as workaround for flickering shadow boundaries on curved surfaces)
    vec3 shadowColor = vec3(1, 1, 1);
    if(dot(-ubo.lightDir, vnInWorldSpace) > 0.07) {
        if(payload.recDepth < ubo.maxRecursions) {
            payload.hitValue = vec3(1.0);
            payload.rayType = RT_SHADOW_TRACE;
            payload.recDepth++;
            traceRayEXT(topLevelAS, gl_RayFlagsOpaqueEXT, 0xFF, 0 /* sbtRecordOffset */, 0 /* sbtRecordStride */, 1 /* missIndex */, origin, tmin * 10,
                    -ubo.lightDir, tmax, 0 /* payload location */);
            payload.recDepth--;
            shadowColor = payload.hitValue;
            payload.rayType = RT_GENERIC;
        }
    }
    else {
        // TODO case where other side of self-shadowing translucent body is not illuminated
        float shadowModulation = pow(mat.transparency, 2);
        shadowColor = mat.transparency < 1.f ? mix(vec3(1, 1, 1), mat.diffuse * shadowModulation, mat.transparency) : vec3(0.4, 0.4, 0.4);
    }

    // Reflection
    vec3 reflectColor = vec3(1, 1, 1);
    float reflectDepth = 0.f;
    if(payload.recDepth < ubo.maxRecursions && mat.reflectivity > 0.f) {

        vec3 dir = reflect(gl_WorldRayDirectionEXT, vnInWorldSpace);

        payload.recDepth += int(mat.rayConsumption);
        payload.refDepth += gl_HitTEXT;
        traceRayEXT(topLevelAS, gl_RayFlagsOpaqueEXT, 0xff, 0 /* sbtRecordOffset */, 0 /* sbtRecordStride */, 0 /* missIndex */, origin, tmin, dir, tmax,
                0 /* payload location */);
        payload.recDepth -= int(mat.rayConsumption);

        reflectColor = payload.hitValue * mat.specular;
        reflectDepth = payload.depth;
    }

    // Refraction
    vec3 refractColor = vec3(1, 1, 1);
    if(payload.recDepth < ubo.maxRecursions && mat.transparency > 0.f) {
        if(frontFacing) {
            float eta = payload.curIOR / mat.ior;
            vec3 dir = refract(gl_WorldRayDirectionEXT, vnInWorldSpace, eta);

            payload.recDepth++;
            payload.curIOR = mat.ior;
            traceRayEXT(topLevelAS, gl_RayFlagsOpaqueEXT, 0xff, 0 /* sbtRecordOffset */, 0 /* sbtRecordStride */, 0 /* missIndex */, origin, tmin, dir, tmax,
                    0 /* payload location */);
            payload.recDepth--;

            refractColor = payload.hitValue;
        }
        else {
            float eta = mat.ior / 1.0;
            vec3 dir = refract(gl_WorldRayDirectionEXT, vnInWorldSpace, eta);

            payload.recDepth++;
            payload.curIOR = 1.0;
            traceRayEXT(topLevelAS, gl_RayFlagsOpaqueEXT, 0xff, 0 /* sbtRecordOffset */, 0 /* sbtRecordStride */, 0 /* missIndex */, origin, tmin, dir, tmax,
                    0 /* payload location */);
            payload.recDepth--;

            vec3 transmittanceModulation = mix(vec3(1, 1, 1), mat.diffuse, log(1 + gl_HitTEXT));
            refractColor = transmittanceModulation * payload.hitValue;
        }
    }

    // Calculate final color
    baseColor *= shadowColor; // only apply direct shadows to base
    baseColor += mat.emission * mat.diffuse;
    float totalContrib = max(mat.transparency, mat.reflectivity);
    vec3 roughCol = mix(refractColor, reflectColor, mat.reflectivity / (mat.transparency + mat.reflectivity));
    payload.hitValue = mix(baseColor, roughCol, totalContrib);

    if(payload.recDepth == 0) {
        payload.hitValue = baseColor;
        payload.normal = vnInWorldSpace;
        payload.roughValue = vec4(roughCol, min((reflectDepth / 50.f) * mat.roughness, mat.roughness / 2.1f));
        payload.reflectContribution = totalContrib;
    }

    payload.depth = gl_HitTEXT;
}
