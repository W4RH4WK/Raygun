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

namespace raygun::render {

class Fade {
  public:
    Fade();
    virtual ~Fade();
    virtual vec4 curColor();
    virtual bool over() const;

  protected:
    double startTime;
};

class FadeIn : public Fade {
  public:
    FadeIn(double duration, vec3 fromColor = vec3(0.f));
    virtual vec4 curColor() override;
    virtual bool over() const override;

  private:
    double duration;
    vec4 fadeCol;
};

class FadeTransition : public Fade {
  public:
    FadeTransition(double halfDuration, std::function<void()> transitionCallback, vec3 transitionColor = vec3(0.f));
    virtual vec4 curColor() override;
    virtual bool over() const override;

  private:
    double halfDuration;
    std::function<void()> transitionCallback;
    vec4 fadeCol;
    bool transitioned = false;
};

} // namespace raygun::render
