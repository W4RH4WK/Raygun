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

    vk::AccelerationStructureInstanceKHR instanceFromEntity(vk::Device device, const Entity& entity, uint32_t instanceId)
    {
        RAYGUN_ASSERT(entity.model->bottomLevelAS);

        vk::AccelerationStructureInstanceKHR instance = {};
        instance.setInstanceCustomIndex(instanceId);
        instance.setMask(0xff);
        instance.setFlags(vk::GeometryInstanceFlagBitsKHR::eTriangleCullDisable);

        // 3x4 row-major affine transformation matrix.
        const auto transform = glm::transpose(entity.globalTransform().toMat4());
        memcpy(&instance.transform, &transform, sizeof(instance.transform));

        const auto blasAddress = device.getAccelerationStructureAddressKHR({vk::AccelerationStructureKHR(*entity.model->bottomLevelAS)});
        instance.setAccelerationStructureReference(blasAddress);

        return instance;
    }

    std::tuple<vk::UniqueAccelerationStructureKHR, vk::UniqueDeviceMemory, gpu::UniqueBuffer>
    createStructureMemoryScratch(const vk::AccelerationStructureCreateInfoKHR& createInfo)
    {
        VulkanContext& vc = RG().vc();

        auto structure = vc.device->createAccelerationStructureKHRUnique(createInfo);

        // allocate memory
        vk::UniqueDeviceMemory memory;
        {
            vk::AccelerationStructureMemoryRequirementsInfoKHR memInfo = {};
            memInfo.setAccelerationStructure(*structure);
            memInfo.setType(vk::AccelerationStructureMemoryRequirementsTypeKHR::eObject);

            auto memoryRequirements = vc.device->getAccelerationStructureMemoryRequirementsKHR(memInfo).memoryRequirements;

            vk::MemoryAllocateFlagsInfo allocInfoFlags = {};
            allocInfoFlags.setFlags(vk::MemoryAllocateFlagBits::eDeviceAddress);

            vk::MemoryAllocateInfo allocInfo = {};
            allocInfo.setAllocationSize(memoryRequirements.size);
            allocInfo.setMemoryTypeIndex(gpu::selectMemoryType(vc.physicalDevice, memoryRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal));
            allocInfo.setPNext(&allocInfoFlags);

            memory = vc.device->allocateMemoryUnique(allocInfo);
        }

        // bind memory
        {
            vk::BindAccelerationStructureMemoryInfoKHR bindInfo = {};
            bindInfo.setAccelerationStructure(*structure);
            bindInfo.setMemory(*memory);

            vc.device->bindAccelerationStructureMemoryKHR(bindInfo);
        }

        // setup scratch buffer
        gpu::UniqueBuffer scratch;
        {
            vk::AccelerationStructureMemoryRequirementsInfoKHR memInfo = {};
            memInfo.setAccelerationStructure(*structure);
            memInfo.setType(vk::AccelerationStructureMemoryRequirementsTypeKHR::eBuildScratch);

            auto memoryRequirements = vc.device->getAccelerationStructureMemoryRequirementsKHR(memInfo).memoryRequirements;

            scratch =
                std::make_unique<gpu::Buffer>(memoryRequirements.size, vk::BufferUsageFlagBits::eRayTracingKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress,
                                              vk::MemoryPropertyFlagBits::eDeviceLocal);
        }

        return {std::move(structure), std::move(memory), std::move(scratch)};
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

    m_instances = gpu::copyToBuffer(instances, vk::BufferUsageFlagBits::eRayTracingKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress);
    m_instances->setName("Instances");

    m_instanceOffsetTable = gpu::copyToBuffer(instanceOffsetTable, vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress);
    m_instanceOffsetTable->setName("Instance Offset Table");

    // setup
    {
        vk::AccelerationStructureCreateGeometryTypeInfoKHR geometryTypeInfo = {};
        geometryTypeInfo.setGeometryType(vk::GeometryTypeKHR::eInstances);
        geometryTypeInfo.setMaxPrimitiveCount((uint32_t)instances.size());

        vk::AccelerationStructureCreateInfoKHR createInfo = {};
        createInfo.setType(vk::AccelerationStructureTypeKHR::eTopLevel);
        createInfo.setFlags(vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace);
        createInfo.setMaxGeometryCount(1);
        createInfo.setPGeometryInfos(&geometryTypeInfo);

        std::tie(m_structure, m_memory, m_scratch) = createStructureMemoryScratch(createInfo);

        vc.setObjectName(*m_structure, "TLAS Structure");
        vc.setObjectName(*m_memory, "TLAS Memory");
        m_scratch->setName("TLAS Scratch");
    }

    // build
    {
        vk::AccelerationStructureGeometryInstancesDataKHR instancesData = {};
        instancesData.setData(m_instances->address());

        vk::AccelerationStructureGeometryDataKHR geometryData = {};
        geometryData.setInstances(instancesData);

        std::array<vk::AccelerationStructureGeometryKHR, 1> geometries;
        geometries[0].setGeometry(geometryData);
        geometries[0].setGeometryType(vk::GeometryTypeKHR::eInstances);
        geometries[0].setFlags(vk::GeometryFlagBitsKHR::eOpaque);

        const auto pGeometires = geometries.data();

        vk::AccelerationStructureBuildGeometryInfoKHR buildInfo = {};
        buildInfo.setType(vk::AccelerationStructureTypeKHR::eTopLevel);
        buildInfo.setDstAccelerationStructure(*m_structure);
        buildInfo.setGeometryCount((uint32_t)geometries.size());
        buildInfo.setPpGeometries(&pGeometires);
        buildInfo.setFlags(vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace);
        buildInfo.setScratchData(m_scratch->address());

        vk::AccelerationStructureBuildOffsetInfoKHR offset = {};
        offset.setPrimitiveCount((uint32_t)instances.size());

        cmd.buildAccelerationStructureKHR(buildInfo, &offset);
    }

    m_descriptorInfo.setAccelerationStructureCount(1);
    m_descriptorInfo.setPAccelerationStructures(&*m_structure);
}

BottomLevelAS::BottomLevelAS(const vk::CommandBuffer& cmd, const Mesh& mesh)
{
    VulkanContext& vc = RG().vc();

    // setup
    {
        vk::AccelerationStructureCreateGeometryTypeInfoKHR geometryTypeInfo = {};
        geometryTypeInfo.setGeometryType(vk::GeometryTypeKHR::eTriangles);
        geometryTypeInfo.setMaxPrimitiveCount((uint32_t)mesh.numFaces());
        geometryTypeInfo.setIndexType(vk::IndexType::eUint32);
        geometryTypeInfo.setMaxVertexCount((uint32_t)mesh.vertices.size());

        vk::AccelerationStructureCreateInfoKHR createInfo = {};
        createInfo.setType(vk::AccelerationStructureTypeKHR::eBottomLevel);
        createInfo.setFlags(vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace);
        createInfo.setMaxGeometryCount(1);
        createInfo.setPGeometryInfos(&geometryTypeInfo);

        std::tie(m_structure, m_memory, m_scratch) = createStructureMemoryScratch(createInfo);

        vc.setObjectName(*m_structure, "BLAS Structure");
        vc.setObjectName(*m_memory, "BLAS Memory");
        m_scratch->setName("BLAS Scratch");
    }

    // build
    {
        vk::AccelerationStructureGeometryTrianglesDataKHR triangles = {};
        triangles.setVertexData(mesh.vertexBufferRef.bufferAddress);
        triangles.setVertexFormat(vk::Format::eR32G32B32Sfloat);
        triangles.setVertexStride(mesh.vertexBufferRef.elementSize);
        triangles.setIndexData(mesh.indexBufferRef.bufferAddress);
        triangles.setIndexType(vk::IndexType::eUint32);

        vk::AccelerationStructureBuildOffsetInfoKHR offsetInfo = {};
        offsetInfo.setPrimitiveCount((uint32_t)mesh.numFaces());
        offsetInfo.setPrimitiveOffset(mesh.indexBufferRef.offsetInBytes);
        offsetInfo.setFirstVertex(mesh.vertexBufferRef.offsetInElements());

        vk::AccelerationStructureGeometryDataKHR geometryData = {};
        geometryData.setTriangles(triangles);

        std::array<vk::AccelerationStructureGeometryKHR, 1> geometries;
        geometries[0].setGeometry(geometryData);
        geometries[0].setGeometryType(vk::GeometryTypeKHR::eTriangles);
        geometries[0].setFlags(vk::GeometryFlagBitsKHR::eOpaque);

        const auto pGeometires = geometries.data();

        vk::AccelerationStructureBuildGeometryInfoKHR buildInfo = {};
        buildInfo.setType(vk::AccelerationStructureTypeKHR::eBottomLevel);
        buildInfo.setFlags(vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace);
        buildInfo.setDstAccelerationStructure(*m_structure);
        buildInfo.setGeometryCount((uint32_t)geometries.size());
        buildInfo.setPpGeometries(&pGeometires);
        buildInfo.setScratchData(m_scratch->address());

        cmd.buildAccelerationStructureKHR(buildInfo, &offsetInfo);
    }
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
