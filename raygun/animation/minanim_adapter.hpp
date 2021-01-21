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

#pragma once

#include "raygun/animation/animation.hpp"
#include "raygun/utils/minanim.hpp"

namespace raygun::animation {

/// This adapter connects the generic MinAnim utility with Raygun's animation
/// system.
class MinAnimTransformAnimation : public ITransformAnimation {
  public:
    MinAnimTransformAnimation(utils::MinAnim<Transform> animation, bool loops = false) : m_animation(animation), m_loops(loops) {}

    Transform evaluate(double timestamp, Transform transform = {}) const override
    {
        m_animation.evaluate(transform, timestamp);
        return transform;
    }

    double duration() const override { return m_animation.duration; }

    bool loops() const override { return m_loops; }

  private:
    utils::MinAnim<Transform> m_animation;
    bool m_loops;
};

} // namespace raygun::animation
