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

#include "raygun/gpu/gpu_utils.hpp"
#include "raygun/raygun.hpp"
#include "raygun/render/model.hpp"
#include "raygun/scene.hpp"

namespace raygun::render {

namespace {

    // See https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/chap33.html#acceleration-structure
    struct VkGeometryInstanceNV {
        float transform[12];
        uint32_t instanceCustomIndex : 24;
        uint32_t mask : 8;
        uint32_t instanceOffset : 24;
        uint32_t flags : 8;
        uint64_t accelerationStructureHandle;
    };
    static_assert(sizeof(VkGeometryInstanceNV) == 64);

    VkGeometryInstanceNV instanceFromEntity(vk::Device device, const Entity& entity, uint32_t instanceId)
    {
        RAYGUN_ASSERT(entity.model->bottomLevelAS);

        VkGeometryInstanceNV instance = {};
        instance.instanceCustomIndex = instanceId;
        instance.instanceOffset = 0;
        instance.mask = 0xff;
        instance.flags = static_cast<uint32_t>(vk::GeometryInstanceFlagBitsNV::eTriangleCullDisable);

        // The expected matrix is a row-major 4x3 matrix.
        const auto transform = glm::transpose(entity.globalTransform().toMat4());
        memcpy(&instance.transform, &transform, sizeof(instance.transform));

        [[maybe_unused]] const auto result = device.getAccelerationStructureHandleNV(*entity.model->bottomLevelAS, sizeof(instance.accelerationStructureHandle),
                                                                                     &instance.accelerationStructureHandle);
        RAYGUN_ASSERT(result == vk::Result::eSuccess);

        return instance;
    }

    std::pair<vk::UniqueAccelerationStructureNV, vk::UniqueDeviceMemory> createAccelerationStructure(const vk::AccelerationStructureInfoNV info)
    {
        VulkanContext& vc = RG().vc();

        vk::AccelerationStructureCreateInfoNV createInfo = {};
        createInfo.setInfo(info);

        auto structure = vc.device->createAccelerationStructureNVUnique(createInfo);

        // allocate memory
        vk::UniqueDeviceMemory memory;
        {
            vk::AccelerationStructureMemoryRequirementsInfoNV memInfo = {};
            memInfo.setAccelerationStructure(*structure);

            const auto memoryRequirements = vc.device->getAccelerationStructureMemoryRequirementsNV(memInfo).memoryRequirements;

            vk::MemoryAllocateInfo allocInfo = {};
            allocInfo.setAllocationSize(memoryRequirements.size);
            allocInfo.setMemoryTypeIndex(gpu::selectMemoryType(vc.physicalDevice, memoryRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal));

            memory = vc.device->allocateMemoryUnique(allocInfo);
        }

        // bind memory
        {
            vk::BindAccelerationStructureMemoryInfoNV bindInfo = {};
            bindInfo.setAccelerationStructure(*structure);
            bindInfo.setMemory(*memory);

            vc.device->bindAccelerationStructureMemoryNV(bindInfo);
        }

        return {std::move(structure), std::move(memory)};
    }

    gpu::UniqueBuffer createScratchBuffer(const vk::AccelerationStructureNV& structure)
    {
        VulkanContext& vc = RG().vc();

        vk::AccelerationStructureMemoryRequirementsInfoNV memInfo = {};
        memInfo.setAccelerationStructure(structure);
        memInfo.setType(vk::AccelerationStructureMemoryRequirementsTypeNV::eBuildScratch);

        const auto scratchSize = vc.device->getAccelerationStructureMemoryRequirementsNV(memInfo).memoryRequirements.size;
        return std::make_unique<gpu::Buffer>(scratchSize, vk::BufferUsageFlagBits::eRayTracingNV, vk::MemoryPropertyFlagBits::eDeviceLocal);
    }

} // namespace

TopLevelAS::TopLevelAS(const vk::CommandBuffer& cmd, const Scene& scene)
{
    VulkanContext& vc = RG().vc();

    std::vector<VkGeometryInstanceNV> instances;
    std::vector<InstanceOffsetTableEntry> instanceOffsetTable;

    // Grab instances from scene.
    scene.root->forEachEntity([&](const Entity& entity) {
        // if set to invisible, do not descend to children
        if(!entity.isVisible()) return false;

        if(entity.transform().isZeroVolume()) return false;

        // if no model, then we skip this, but might still render children
        if(!entity.model) return true;

        const auto instance = instanceFromEntity(*vc.device, entity, (uint32_t)instances.size());
        instances.push_back(instance);

        const auto& vertexBufferRef = entity.model->mesh->vertexBufferRef;
        const auto& indexBufferRef = entity.model->mesh->indexBufferRef;
        const auto& materialBufferRef = entity.model->materialBufferRef;

        auto& entry = instanceOffsetTable.emplace_back();
        entry.vertexBufferOffset = (uint32_t)(vertexBufferRef.offset / vertexBufferRef.elementSize);
        entry.indexBufferOffset = (uint32_t)(indexBufferRef.offset / indexBufferRef.elementSize);
        entry.materialBufferOffset = (uint32_t)(materialBufferRef.offset / materialBufferRef.elementSize);

        return true;
    });

    // Setup acceleration structure.
    {
        m_info.setType(vk::AccelerationStructureTypeNV::eTopLevel);
        m_info.setInstanceCount((uint32_t)instances.size());
        m_info.setFlags(vk::BuildAccelerationStructureFlagBitsNV::ePreferFastTrace);

        vk::AccelerationStructureCreateInfoNV createInfo = {};
        createInfo.setInfo(m_info);

        std::tie(m_structure, m_memory) = createAccelerationStructure(m_info);
        vc.setObjectName(*m_structure, "TLAS");
        vc.setObjectName(*m_memory, "TLAS");
    }

    m_scratch = createScratchBuffer(*m_structure);
    m_scratch->setName("TLAS");

    m_instances = gpu::copyToBuffer(instances, vk::BufferUsageFlagBits::eRayTracingNV);
    m_instanceOffsetTable = gpu::copyToBuffer(instanceOffsetTable, vk::BufferUsageFlagBits::eStorageBuffer);

    cmd.buildAccelerationStructureNV(m_info, *m_instances, 0, false, *m_structure, nullptr, *m_scratch, 0);

    m_descriptorInfo.setAccelerationStructureCount(1);
    m_descriptorInfo.setPAccelerationStructures(&*m_structure);
}

BottomLevelAS::BottomLevelAS(const vk::CommandBuffer& cmd, const Mesh& mesh)
{
    VulkanContext& vc = RG().vc();

    vk::GeometryTrianglesNV triangles = {};
    triangles.setVertexData(mesh.vertexBufferRef.buffer);
    triangles.setVertexOffset(mesh.vertexBufferRef.offset);
    triangles.setVertexCount(mesh.vertexBufferRef.elementCount());
    triangles.setVertexStride(mesh.vertexBufferRef.elementSize);
    triangles.setVertexFormat(vk::Format::eR32G32B32Sfloat);
    triangles.setIndexData(mesh.indexBufferRef.buffer);
    triangles.setIndexOffset(mesh.indexBufferRef.offset);
    triangles.setIndexCount(mesh.indexBufferRef.elementCount());
    triangles.setIndexType(vk::IndexType::eUint32);

    vk::GeometryDataNV geometryData = {};
    geometryData.setTriangles(triangles);

    vk::GeometryNV geometry = {};
    geometry.setGeometry(geometryData);
    geometry.setGeometryType(vk::GeometryTypeNV::eTriangles);
    geometry.setFlags(vk::GeometryFlagBitsNV::eOpaque);

    vk::AccelerationStructureInfoNV info = {};
    info.setType(vk::AccelerationStructureTypeNV::eBottomLevel);
    info.setGeometryCount(1);
    info.setPGeometries(&geometry);
    info.setFlags(vk::BuildAccelerationStructureFlagBitsNV::ePreferFastTrace);

    vk::AccelerationStructureCreateInfoNV createInfo = {};
    createInfo.setInfo(info);

    std::tie(m_structure, m_memory) = createAccelerationStructure(info);
    vc.setObjectName(*m_structure, "BLAS");
    vc.setObjectName(*m_memory, "BLAS");

    m_scratch = createScratchBuffer(*m_structure);
    m_scratch->setName("BLAS");

    cmd.buildAccelerationStructureNV(info, nullptr, 0, false, *m_structure, nullptr, *m_scratch, 0);
}

void accelerationStructureBarrier(const vk::CommandBuffer& cmd)
{
    vk::MemoryBarrier memoryBarrier = {};
    memoryBarrier.setSrcAccessMask(vk::AccessFlagBits::eAccelerationStructureReadNV | vk::AccessFlagBits::eAccelerationStructureReadNV);
    memoryBarrier.setDstAccessMask(vk::AccessFlagBits::eAccelerationStructureReadNV | vk::AccessFlagBits::eAccelerationStructureReadNV);

    cmd.pipelineBarrier(vk::PipelineStageFlagBits::eAccelerationStructureBuildNV, vk::PipelineStageFlagBits::eAccelerationStructureBuildNV, {}, {memoryBarrier},
                        {}, {});
}

} // namespace raygun::render
