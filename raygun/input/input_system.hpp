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

namespace raygun::input {

/// Input struct defining all possible actions. This struct is passed on to the
/// game logic.
struct Input {
    const float DEADZONE = 0.5f;

    vec2 dir = {0.0f, 0.0f};

    bool ok = false;
    bool cancel = false;
    bool reload = false;

    bool left() const { return dir.x < -DEADZONE; }

    bool right() const { return dir.x > DEADZONE; }

    bool up() const { return dir.y > DEADZONE; }

    bool down() const { return dir.y < -DEADZONE; }
};

class InputSystem {
  public:
    InputSystem();

    Input handleEvents();
};

using UniqueInputSystem = std::unique_ptr<InputSystem>;

} // namespace raygun::input
