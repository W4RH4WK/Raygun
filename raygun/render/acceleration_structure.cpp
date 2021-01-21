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

#include "raygun/render/acceleration_structure.hpp"

#include "raygun/gpu/gpu_utils.hpp"
#include "raygun/raygun.hpp"
#include "raygun/render/model.hpp"
#include "raygun/scene.hpp"

namespace raygun::render {

namespace {

    vk::AccelerationStructureInstanceKHR instanceFromEntity(vk::Device device, const Entity& entity, uint32_t instanceId)
    {
        RAYGUN_ASSERT(entity.model->bottomLevelAS);

        vk::AccelerationStructureInstanceKHR instance = {};
        instance.setInstanceCustomIndex(instanceId);
        instance.setMask(0xff);
        instance.setFlags(vk::GeometryInstanceFlagBitsKHR::eTriangleCullDisable);

        // 3x4 row-major affine transformation matrix.
        const auto transform = glm::transpose(entity.globalTransform().toMat4());
        instance.transform.matrix = *reinterpret_cast<const vk::ArrayWrapper2D<float, 3, 4>*>(&transform);

        const auto blasAddress = device.getAccelerationStructureAddressKHR({vk::AccelerationStructureKHR(*entity.model->bottomLevelAS)});
        instance.setAccelerationStructureReference(blasAddress);

        return instance;
    }

} // namespace

TopLevelAS::TopLevelAS(const vk::CommandBuffer& cmd, const Scene& scene)
{
    VulkanContext& vc = RG().vc();

    std::vector<vk::AccelerationStructureInstanceKHR> instances;
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
        entry.vertexBufferOffset = vertexBufferRef.offsetInElements();
        entry.indexBufferOffset = indexBufferRef.offsetInElements();
        entry.materialBufferOffset = materialBufferRef.offsetInElements();

        return true;
    });

    m_instances = gpu::copyToBuffer(instances, vk::BufferUsageFlagBits::eShaderDeviceAddress);
    m_instances->setName("TLAS Instances");

    m_instanceOffsetTable = gpu::copyToBuffer(instanceOffsetTable, vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress);
    m_instanceOffsetTable->setName("Instance Offset Table");

    vk::AccelerationStructureGeometryInstancesDataKHR instancesData = {};
    instancesData.setData(m_instances->address());

    vk::AccelerationStructureGeometryDataKHR geometryData = {};
    geometryData.setInstances(instancesData);

    vk::AccelerationStructureGeometryKHR geometry = {};
    geometry.setGeometryType(vk::GeometryTypeKHR::eInstances);
    geometry.setGeometry(geometryData);

    vk::AccelerationStructureBuildGeometryInfoKHR buildInfo = {};
    buildInfo.setPGeometries(&geometry);
    buildInfo.setGeometryCount(1);
    buildInfo.setType(vk::AccelerationStructureTypeKHR::eTopLevel);

    const auto buildSize =
        vc.device->getAccelerationStructureBuildSizesKHR(vk::AccelerationStructureBuildTypeKHR::eDevice, buildInfo, (uint32_t)instances.size());

    vk::AccelerationStructureCreateInfoKHR createInfo = {};
    createInfo.setType(vk::AccelerationStructureTypeKHR::eTopLevel);
    createInfo.setSize(buildSize.accelerationStructureSize);

    m_structureMemory = std::make_unique<gpu::Buffer>(buildSize.accelerationStructureSize,
                                                      vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress,
                                                      vk::MemoryPropertyFlagBits::eDeviceLocal);
    m_structureMemory->setName("TLAS Structure Memory");
    createInfo.setBuffer(*m_structureMemory);

    m_structure = vc.device->createAccelerationStructureKHRUnique(createInfo);
    vc.setObjectName(*m_structure, "TLAS Structure");
    buildInfo.setDstAccelerationStructure(*m_structure);

    m_scratch =
        std::make_unique<gpu::Buffer>(buildSize.buildScratchSize, vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eStorageBuffer,
                                      vk::MemoryPropertyFlagBits::eDeviceLocal);
    m_scratch->setName("TLAS Scratch");
    buildInfo.setScratchData(m_scratch->address());

    vk::AccelerationStructureBuildRangeInfoKHR offset = {};
    offset.setPrimitiveCount((uint32_t)instances.size());

    cmd.buildAccelerationStructuresKHR(buildInfo, &offset);

    m_descriptorInfo.setAccelerationStructureCount(1);
    m_descriptorInfo.setPAccelerationStructures(&*m_structure);
}

BottomLevelAS::BottomLevelAS(const vk::CommandBuffer& cmd, const Mesh& mesh)
{
    VulkanContext& vc = RG().vc();

    vk::AccelerationStructureGeometryTrianglesDataKHR triangles = {};
    triangles.setVertexFormat(vk::Format::eR32G32B32Sfloat);
    triangles.setVertexData(mesh.vertexBufferRef.bufferAddress);
    triangles.setVertexStride(mesh.vertexBufferRef.elementSize);
    triangles.setIndexType(vk::IndexType::eUint32);
    triangles.setIndexData(mesh.indexBufferRef.bufferAddress);
    triangles.setMaxVertex((uint32_t)mesh.vertices.size());

    vk::AccelerationStructureGeometryDataKHR geometryData = {};
    geometryData.setTriangles(triangles);

    vk::AccelerationStructureGeometryKHR geometry = {};
    geometry.setGeometryType(vk::GeometryTypeKHR::eTriangles);
    geometry.setGeometry(geometryData);

    vk::AccelerationStructureBuildGeometryInfoKHR buildInfo = {};
    buildInfo.setType(vk::AccelerationStructureTypeKHR::eBottomLevel);
    buildInfo.setFlags(vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace);
    buildInfo.setPGeometries(&geometry);
    buildInfo.setGeometryCount(1);

    const auto buildSize =
        vc.device->getAccelerationStructureBuildSizesKHR(vk::AccelerationStructureBuildTypeKHR::eDevice, buildInfo, (uint32_t)mesh.numFaces());

    vk::AccelerationStructureCreateInfoKHR createInfo = {};
    createInfo.setType(vk::AccelerationStructureTypeKHR::eBottomLevel);
    createInfo.setSize(buildSize.accelerationStructureSize);

    m_structureMemory = std::make_unique<gpu::Buffer>(buildSize.accelerationStructureSize,
                                                      vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress,
                                                      vk::MemoryPropertyFlagBits::eDeviceLocal);
    m_structureMemory->setName("BLAS Structure Memory");
    createInfo.setBuffer(*m_structureMemory);

    m_structure = vc.device->createAccelerationStructureKHRUnique(createInfo);
    vc.setObjectName(*m_structure, "BLAS Structure");
    buildInfo.setDstAccelerationStructure(*m_structure);

    m_scratch =
        std::make_unique<gpu::Buffer>(buildSize.buildScratchSize, vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eStorageBuffer,
                                      vk::MemoryPropertyFlagBits::eDeviceLocal);
    m_scratch->setName("BLAS Scratch");
    buildInfo.setScratchData(m_scratch->address());

    vk::AccelerationStructureBuildRangeInfoKHR offset = {};
    offset.setPrimitiveCount((uint32_t)mesh.numFaces());
    offset.setPrimitiveOffset(mesh.indexBufferRef.offsetInBytes);
    offset.setFirstVertex(mesh.vertexBufferRef.offsetInElements());

    cmd.buildAccelerationStructuresKHR(buildInfo, &offset);
}

void accelerationStructureBarrier(const vk::CommandBuffer& cmd)
{
    vk::MemoryBarrier memoryBarrier = {};
    memoryBarrier.setSrcAccessMask(vk::AccessFlagBits::eAccelerationStructureReadKHR | vk::AccessFlagBits::eAccelerationStructureReadKHR);
    memoryBarrier.setDstAccessMask(vk::AccessFlagBits::eAccelerationStructureReadKHR | vk::AccessFlagBits::eAccelerationStructureReadKHR);

    cmd.pipelineBarrier(vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR, vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR, {},
                        {memoryBarrier}, {}, {});
}

} // namespace raygun::render
