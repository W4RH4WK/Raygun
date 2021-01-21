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

namespace raygun::utils {

/// MinAnim is a very basic, but generic animation sequencer. It does not
/// maintain state directly. It is based on this blog post:
/// https://bollu.github.io/mathemagic/declarative/index.html
///
/// Lifetime of (internal) objects is managed automatically. This is done via
/// lambdas, capturing objects by value. The evaluate member function takes an
/// instance of T, the data, that gets mutated by the animation.
template<typename T>
struct MinAnim {
    using type = T;

    const double duration = 0.0;

    using Operation = std::function<void(T& data, double t, double tstart, double duration)>;
    const Operation operation = nullptr;

    void evaluate(T& data, double t, double tstart = 0.0) const
    {
        if(operation) {
            operation(data, t, tstart, duration);
        }
    }

    /// Delay the following animations, no side-effects occur.
    static MinAnim delay(double duration) { return {duration}; }

    /// Sets data to the given value.
    static MinAnim set(T value)
    {
        return {0.0, [=](T& data, double, double, double) { data = value; }};
    }

    /// Sets a member of data to the given value.
    template<typename V>
    static MinAnim set(V T::*field, V value)
    {
        return {0.0, [=](T& data, double, double, double) { data.*field = value; }};
    }

    /// Modify data with the given callable.
    template<typename F>
    static MinAnim modify(F f)
    {
        return {0.0, [=](T& data, double, double, double) { f(data); }};
    }

    /// Modify data with the given callable across a given duration. The
    /// callable will be called like this: f(data, percent) where percent is
    /// [0.0, 1.0].
    template<typename F>
    static MinAnim modify(double duration, F f)
    {
        return {duration, [=](T& data, double t, double tstart, double duration) {
                    const auto percent = (t - tstart) / duration;
                    f(data, std::clamp(percent, 0.0, 1.0));
                }};
    }

    /// Linearly interpolate data across a given duration.
    static MinAnim lerp(double duration, T end)
    {
        return modify(duration, [=](T& data, double percent) { data = glm::mix(data, end, percent); });
    }

    /// Linearly interpolate a member of data across a given duration.
    template<typename V>
    static MinAnim lerp(double duration, V T::*field, V end)
    {
        return modify(duration, [=](T& data, double percent) { data.*field = glm::mix(data.*field, end, percent); });
    }

    /// Sequence combinator allowing for animations to be chained.
    MinAnim seq(MinAnim next) const
    {
        return {duration + next.duration, [=, *this](T& data, double t, double tstart, double) {
                    // Always execute the first (this) animation as its results
                    // may be needed for the following ones.
                    evaluate(data, t, tstart);

                    if(t >= tstart + duration) {
                        next.evaluate(data, t, tstart + duration);
                    }
                }};
    }

    /// Parallel combinator allowing for animations to be run in parallel.
    MinAnim par(MinAnim other) const
    {
        return {std::max(duration, other.duration), [=, *this](T& data, double t, double tstart, double) {
                    if(t >= tstart) {
                        evaluate(data, t, tstart);
                        other.evaluate(data, t, tstart);
                    }
                }};
    }
};

} // namespace raygun::utils
