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
#include "raygun/render/vertex.hpp"

namespace raygun::render {

struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    gpu::BufferRef vertexBufferRef;
    gpu::BufferRef indexBufferRef;

    size_t numFaces() const { return indices.size() / 3; }

    vec3 center() const;

    struct Bounds {
        vec3 lower, upper;
    };

    Bounds bounds() const;
    float width() const;

    /// Merges the given Mesh into this one, material indices remain untouched.
    void merge(const Mesh& other);

    void forEachFace(std::function<void(const Vertex&, const Vertex&, const Vertex&)>) const;
};

} // namespace raygun::render
