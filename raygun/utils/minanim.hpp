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

namespace raygun::utils::minanim {

/// MinAnim is a very basic, but generic animation sequencer. It does not
/// maintain state directly. It is based on this blog post:
/// https://bollu.github.io/mathemagic/declarative/index.html
///
/// Lifetime of (internal) objects is managed automatically. This is done via
/// lambdas, capturing objects by value.
struct MinAnim {
    const double duration = 0;

    using Operation = std::function<void(double t, double tstart, double duration)>;
    const Operation operation = nullptr;

    void evaluate(double t, double tstart = 0) const
    {
        if(operation) {
            operation(t, tstart, duration);
        }
    }

    /// Sequence combinator allowing for animations to be chained.
    MinAnim seq(MinAnim next) const
    {
        return {duration + next.duration, [=, *this](double t, double tstart, double) {
                    // Always execute the first (this) animation as its results may
                    // be needed for the following ones.
                    evaluate(t, tstart);

                    if(t >= tstart + duration) {
                        next.evaluate(t, tstart + duration);
                    }
                }};
    }

    /// Parallel combinator allowing for animations to be run in parallel.
    MinAnim par(MinAnim other) const
    {
        return {std::max(duration, other.duration), [=, *this](double t, double tstart, double) {
                    if(t >= tstart) {
                        evaluate(t, tstart);
                        other.evaluate(t, tstart);
                    }
                }};
    }
};

/// Delay the following animations, no side-effects occur.
static inline MinAnim delay(double duration)
{
    return {duration};
}

/// Sets target to the given value.
template<typename T>
MinAnim set(T& target, T value)
{
    return {0, [=, &target](double, double, double) { target = value; }};
}

/// Linearly interpolates target between the previous and end value.
template<typename T>
MinAnim lerp(T& target, T end, double duration)
{
    using namespace glm;

    return {duration, [=, &target](double t, double tstart, double duration) {
                const auto factor = (t - tstart) / duration;
                target = mix(target, end, clamp(factor, 0.0, 1.0));
            }};
}

} // namespace raygun::utils::minanim
