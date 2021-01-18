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

#include "raygun/transform.hpp"

namespace raygun::animation {

/// This interfaces specifics the requirements of an animation clip that
/// modifies an entity's transform.
class ITransformAnimation {
  public:
    virtual Transform evaluate(double timestamp, Transform transform = {}) const = 0;
    virtual double duration() const = 0;
    virtual bool loops() const = 0;
    virtual ~ITransformAnimation() {}
};

class ScaleAnimation : public ITransformAnimation {
  public:
    ScaleAnimation(vec3 start, vec3 end, double duration, bool loops = false) : m_start(start), m_end(end), m_duration(duration), m_loops(loops) {}

    Transform evaluate(double timestamp, Transform transform = {}) const override
    {
        const auto factor = std::clamp(timestamp / m_duration, 0.0, 1.0);
        transform.scaling = glm::mix(m_start, m_end, factor);
        return transform;
    }

    double duration() const override { return m_duration; }

    bool loops() const override { return m_loops; }

  private:
    vec3 m_start, m_end;
    double m_duration;
    bool m_loops;
};

} // namespace raygun::animation
