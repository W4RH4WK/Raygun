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

#include "raygun/render/model.hpp"

namespace raygun::render {

vec3 Mesh::center() const
{
    auto sum = std::accumulate(indices.begin(), indices.end(), vec3{0.0f}, [&](auto sum, auto index) { return sum + vertices[index].position; });
    return sum / (float)indices.size();
}

Mesh::Bounds Mesh::bounds() const
{
    vec3 lower{std::numeric_limits<float>::max()};
    vec3 upper{std::numeric_limits<float>::lowest()};

    for(auto v: vertices) {
        lower = min(v.position, lower);
        upper = max(v.position, upper);
    }

    return {lower, upper};
}

float Mesh::width() const
{
    auto b = bounds();
    return b.upper.x - b.lower.x;
}

void Mesh::merge(const Mesh& other)
{
    // Indices need to be modified as vertices get merged.
    const auto indexOffset = (uint32_t)vertices.size();
    const auto updateIndex = [=](auto index) { return index + indexOffset; };

    indices.reserve(indices.size() + other.indices.size());
    std::transform(other.indices.begin(), other.indices.end(), std::back_inserter(indices), updateIndex);

    vertices.insert(vertices.end(), other.vertices.begin(), other.vertices.end());
}

void Mesh::forEachFace(std::function<void(const Vertex&, const Vertex&, const Vertex&)> action) const
{
    for(auto i = 0u; i < indices.size(); i += 3) {
        const auto& v0 = vertices[indices[i + 0]];
        const auto& v1 = vertices[indices[i + 1]];
        const auto& v2 = vertices[indices[i + 2]];

        action(v0, v1, v2);
    }
}

} // namespace raygun::render
