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

#include "raygun/render/fade.hpp"

#include "raygun/raygun.hpp"

namespace raygun::render {

raygun::render::Fade::Fade() : startTime(RG().time()) {}

Fade::~Fade() {}

raygun::vec4 Fade::curColor()
{
    return vec4(0.f);
}

bool Fade::over() const
{
    return true;
}

/////////////////////////////////////// FadeIn

FadeIn::FadeIn(double duration, vec3 fromColor) : duration(duration), fadeCol(fromColor, 1.f) {}

raygun::vec4 FadeIn::curColor()
{
    double progress = (RG().time() - startTime) / duration;
    fadeCol.a = float(1 - std::clamp(progress, 0., 1.));
    return fadeCol;
}

bool FadeIn::over() const
{
    return (RG().time() - startTime) > duration;
}

/////////////////////////////////////// FadeTransition

FadeTransition::FadeTransition(double halfDuration, std::function<void()> transitionCallback, vec3 transitionColor)
    : halfDuration(halfDuration)
    , transitionCallback(transitionCallback)
    , fadeCol(transitionColor, 0.f)
{
}

raygun::vec4 FadeTransition::curColor()
{
    double progress = (RG().time() - startTime) / halfDuration;

    if(!transitioned) {
        fadeCol.a = float(std::clamp(progress, 0., 1.));
        if(progress >= 1) {
            transitioned = true;
            transitionCallback();
        }
    }
    else {
        fadeCol.a = float(std::clamp(2 - progress, 0., 1.));
    }

    return fadeCol;
}

bool FadeTransition::over() const
{
    return (RG().time() - startTime) > halfDuration * 2;
}

} // namespace raygun::render
