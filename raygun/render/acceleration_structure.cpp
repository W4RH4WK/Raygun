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

#include "raygun/render/acceleration_structure.hpp"

#include <nv_helpers_vk/BottomLevelASGenerator.h>
#include <nv_helpers_vk/TopLevelASGenerator.h>

#include "raygun/gpu/gpu_utils.hpp"
#include "raygun/raygun.hpp"
#include "raygun/render/model.hpp"
#include "raygun/scene.hpp"

namespace raygun::render {

struct Dummy {
    Dummy(const vk::CommandBuffer& cmd)
    {
        auto mesh = std::make_shared<Mesh>();

        auto& vertices = mesh->vertices;
        vertices.resize(3);
        vertices[0].position = {0.0f, 0.0f, 0.0f};
        vertices[0].normal = {0.0f, 0.0f, 1.0f};
        vertices[1].position = {1.0f, 0.0f, 0.0f};
        vertices[1].normal = {0.0f, 0.0f, 1.0f};
        vertices[2].position = {1.0f, 1.0f, 0.0f};
        vertices[2].normal = {0.0f, 0.0f, 1.0f};

        auto& indices = mesh->indices;
        indices.resize(3);
        indices[0] = 0;
        indices[1] = 1;
        indices[2] = 2;

        vertexBuffer = std::make_unique<gpu::Buffer>(vertices.size() * sizeof(render::Vertex),
                                                     vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eStorageBuffer,
                                                     vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

        indexBuffer =
            std::make_unique<gpu::Buffer>(indices.size() * sizeof(uint32_t), vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eStorageBuffer,
                                          vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

        memcpy(vertexBuffer->map(), vertices.data(), vertices.size() * sizeof(vertices[0]));
        vertexBuffer->unmap();

        memcpy(indexBuffer->map(), indices.data(), indices.size() * sizeof(indices[0]));
        indexBuffer->unmap();

        mesh->vertexBufferRef.buffer = *vertexBuffer;
        mesh->vertexBufferRef.offset = 0;
        mesh->vertexBufferRef.size = vertices.size() * sizeof(vertices[0]);
        mesh->vertexBufferRef.elementSize = sizeof(vertices[0]);

        mesh->indexBufferRef.buffer = *indexBuffer;
        mesh->indexBufferRef.offset = 0;
        mesh->indexBufferRef.size = indices.size() * sizeof(indices[0]);
        mesh->indexBufferRef.elementSize = sizeof(indices[0]);

        model.mesh = mesh;
        model.bottomLevelAS = std::make_unique<BottomLevelAS>(cmd, *mesh);
    }

    render::Model model;
    gpu::UniqueBuffer vertexBuffer;
    gpu::UniqueBuffer indexBuffer;
};

TopLevelAS::TopLevelAS(const vk::CommandBuffer& cmd, Scene& scene)
{
    VulkanContext& vc = RG().vc();

    nv_helpers_vk::TopLevelASGenerator gen;

    std::vector<InstanceOffsetTableEntry> instanceOffsetTable;

    scene.root->forEachEntity([&](const Entity& entity) {
        // if set to invisible, do not descend to children
        if(!entity.isVisible()) return false;

        if(entity.transform().isZeroVolume()) return false;

        // if no model, then we skip this, but might still render children
        if(!entity.model) return true;

        const auto instanceID = (uint32_t)instanceOffsetTable.size();

        // All instances currently use the same hit group.
        gen.AddInstance(entity.model->bottomLevelAS->structure(), entity.globalTransform().toMat4(), instanceID, 0);

        const auto& vertexBufferRef = entity.model->mesh->vertexBufferRef;
        const auto& indexBufferRef = entity.model->mesh->indexBufferRef;
        const auto& materialBufferRef = entity.model->materialBufferRef;

        auto& entry = instanceOffsetTable.emplace_back();
        entry.vertexBufferOffset = (uint32_t)(vertexBufferRef.offset / vertexBufferRef.elementSize);
        entry.indexBufferOffset = (uint32_t)(indexBufferRef.offset / indexBufferRef.elementSize);
        entry.materialBufferOffset = (uint32_t)(materialBufferRef.offset / materialBufferRef.elementSize);
        return true;
    });

    // Since we need at least one instance, a dummy is added when necessary.
    if(instanceOffsetTable.empty()) {
        m_dummy = new Dummy(cmd);

        // No ray should be able to hit this.
        const auto transform = glm::scale(mat4{1.0f}, zero());
        gen.AddInstance(m_dummy->model.bottomLevelAS->structure(), transform, 1, 0);

        instanceOffsetTable.push_back({});
    }

    {
        const auto structure = gen.CreateAccelerationStructure(*vc.device);
        m_structure = gpu::wrapUnique<vk::AccelerationStructureNV>(structure, *vc.device);
    }

    m_info.setAccelerationStructureCount(1);
    m_info.setPAccelerationStructures(&*m_structure);

    vk::DeviceSize scratchSize, resultSize, instanceBufferSize;
    gen.ComputeASBufferSizes(*vc.device, *m_structure, &scratchSize, &resultSize, &instanceBufferSize);

    m_scratch = std::make_unique<gpu::Buffer>(scratchSize, vk::BufferUsageFlagBits::eRayTracingNV, vk::MemoryPropertyFlagBits::eDeviceLocal);

    m_result = std::make_unique<gpu::Buffer>(resultSize, vk::BufferUsageFlagBits::eRayTracingNV, vk::MemoryPropertyFlagBits::eDeviceLocal);

    m_instances = std::make_unique<gpu::Buffer>(instanceBufferSize, vk::BufferUsageFlagBits::eRayTracingNV,
                                                vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

    gen.Generate(*vc.device, cmd, *m_structure, *m_scratch, 0, m_result->memory(), *m_instances, m_instances->memory());

    // Setup instance offset table.
    // TODO: Barrier required?
    {
        const auto bufferSize = instanceOffsetTable.size() * sizeof(instanceOffsetTable[0]);

        m_instanceOffsetTable = std::make_unique<gpu::Buffer>(bufferSize, vk::BufferUsageFlagBits::eStorageBuffer,
                                                              vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

        memcpy(m_instanceOffsetTable->map(), instanceOffsetTable.data(), bufferSize);
        m_instanceOffsetTable->unmap();
    }
}

TopLevelAS::~TopLevelAS()
{
    if(m_dummy) delete m_dummy;
}

BottomLevelAS::BottomLevelAS(const vk::CommandBuffer& cmd, const Mesh& mesh)
{
    VulkanContext& vc = RG().vc();

    nv_helpers_vk::BottomLevelASGenerator gen;

    const auto vertexCount = (uint32_t)mesh.vertexBufferRef.elementCount();
    const auto indexCount = (uint32_t)mesh.indexBufferRef.elementCount();
    const auto vertexSize = mesh.vertexBufferRef.elementSize;

    gen.AddVertexBuffer(mesh.vertexBufferRef.buffer, mesh.vertexBufferRef.offset, vertexCount, vertexSize, mesh.indexBufferRef.buffer,
                        mesh.indexBufferRef.offset, indexCount, nullptr, 0);

    {
        const auto structure = gen.CreateAccelerationStructure(*vc.device);
        m_structure = gpu::wrapUnique<vk::AccelerationStructureNV>(structure, *vc.device);
    }

    vk::DeviceSize scratchSize, resultSize;
    gen.ComputeASBufferSizes(*vc.device, *m_structure, &scratchSize, &resultSize);

    m_scratch = std::make_unique<gpu::Buffer>(scratchSize, vk::BufferUsageFlagBits::eRayTracingNV, vk::MemoryPropertyFlagBits::eDeviceLocal);

    m_result = std::make_unique<gpu::Buffer>(resultSize, vk::BufferUsageFlagBits::eRayTracingNV, vk::MemoryPropertyFlagBits::eDeviceLocal);

    gen.Generate(*vc.device, cmd, *m_structure, *m_scratch, 0, m_result->memory());
}

} // namespace raygun::render
