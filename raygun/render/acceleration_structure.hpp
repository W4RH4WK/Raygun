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

#include "raygun/gpu/gpu_buffer.hpp"
#include "raygun/render/mesh.hpp"

namespace raygun {
struct Scene;
}

namespace raygun::render {

struct Dummy;

class TopLevelAS {
  public:
    TopLevelAS(const vk::CommandBuffer& cmd, const Scene& scene);
    ~TopLevelAS();

    const vk::AccelerationStructureNV& structure() const { return *m_structure; }

    const gpu::Buffer& instanceOffsetTable() const { return *m_instanceOffsetTable; }

    const vk::WriteDescriptorSetAccelerationStructureNV& info() const { return m_info; }

  private:
    vk::UniqueAccelerationStructureNV m_structure;
    gpu::UniqueBuffer m_scratch;
    gpu::UniqueBuffer m_result;
    gpu::UniqueBuffer m_instances;

    /// Shader relevant data is commonly combined into large buffers (e.g. one
    /// vertex buffer for all vertices). In order to find the data corresponding
    /// to a specific primitive, the offsets in these buffers must be known.
    /// This lookup table provides the needed offsets for each instance.
    gpu::UniqueBuffer m_instanceOffsetTable;

    vk::WriteDescriptorSetAccelerationStructureNV m_info = {};

    /// At least one entry in the TopLevelAS is required. This dummy is added if
    /// necessary.
    std::unique_ptr<Dummy> m_dummy;
};

using UniqueTopLevelAS = std::unique_ptr<TopLevelAS>;

class BottomLevelAS {
  public:
    BottomLevelAS(const vk::CommandBuffer& cmd, const Mesh& mesh);

    const vk::AccelerationStructureNV& structure() const { return *m_structure; }

  private:
    vk::UniqueAccelerationStructureNV m_structure;
    gpu::UniqueBuffer m_scratch;
    gpu::UniqueBuffer m_result;
};

using UniqueBottomLevelAS = std::unique_ptr<BottomLevelAS>;

struct InstanceOffsetTableEntry {
    using uint = uint32_t;
#include "resources/shaders/instance_offset_table.def"
};

} // namespace raygun::render
