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

#include "raygun/input/input_system.hpp"

#include "raygun/gpu/shader.hpp"
#include "raygun/logging.hpp"
#include "raygun/raygun.hpp"

namespace raygun::input {

InputSystem::InputSystem()
{
    RAYGUN_INFO("Input system initialized");
}

Input InputSystem::handleEvents()
{
    const auto& window = RG().window().window();
    const auto& pressed = [&](auto key) { return glfwGetKey(window, key) == GLFW_PRESS; };

    Input input;
    input.ok = pressed(GLFW_KEY_ENTER);
    input.cancel = pressed(GLFW_KEY_ESCAPE);
    input.dir.x = (pressed(GLFW_KEY_LEFT) ? -1.0f : 0.0f) + (pressed(GLFW_KEY_RIGHT) ? 1.0f : 0.0f);
    input.dir.y = (pressed(GLFW_KEY_DOWN) ? -1.0f : 0.0f) + (pressed(GLFW_KEY_UP) ? 1.0f : 0.0f);

    // developer bindings
    {
        if(pressed(GLFW_KEY_F5)) {
            input.reload = true;
        }

        if(pressed(GLFW_KEY_F6)) {
            RG().resourceManager().clearShaderCache();
            gpu::recompileAllShaders();
            RG().renderSystem().reload();
        }

        if(pressed(GLFW_KEY_F10)) {
            RG().quit();
        }
    }

    if(glm::length(input.dir) > 1.0f) {
        input.dir = glm::normalize(input.dir);
    }

    return input;
}

} // namespace raygun::input
