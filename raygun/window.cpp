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

#include "raygun/window.hpp"

#include "raygun/config.hpp"
#include "raygun/gpu/gpu_utils.hpp"
#include "raygun/logging.hpp"
#include "raygun/raygun.hpp"

namespace raygun {

Window::Window(string_view title)
{
    const auto& config = RG().config();

    if(config.headless) return;

    int width = config.width;
    int height = config.height;
    auto monitor = glfwGetPrimaryMonitor();
    auto mode = glfwGetVideoMode(monitor);

    switch(config.fullscreen) {
    case Config::Fullscreen::Fullscreen:
        break;

    case Config::Fullscreen::Borderless:
        width = mode->width;
        height = mode->height;
        glfwWindowHint(GLFW_AUTO_ICONIFY, GLFW_FALSE);
        glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
        [[fallthrough]];

    case Config::Fullscreen::Window:
        monitor = nullptr;
        break;
    }

    // HACK: see https://github.com/glfw/glfw/issues/527
    if(config.fullscreen == Config::Fullscreen::Borderless) {
        height++;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    glfwWindowHint(GLFW_RED_BITS, mode->redBits);
    glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
    glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
    glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

    m_window = glfwCreateWindow(width, height, title.data(), monitor, nullptr);
    if(m_window) {
        RAYGUN_INFO("Window initialized");
    }
    else {
        RAYGUN_FATAL("Unable to initialize window");
    }

    glfwSetWindowIcon(m_window, 1, &m_windowIcon.image);

    glfwSetFramebufferSizeCallback(m_window, [](auto, auto, auto) { RG().window().m_resized = true; });
}

Window::~Window()
{
    glfwDestroyWindow(m_window);
}

vk::Extent2D Window::size() const
{
    int w = 0, h = 0;
    glfwGetFramebufferSize(m_window, &w, &h);
    return {(uint32_t)w, (uint32_t)h};
}

bool Window::minimized() const
{
    return glfwGetWindowAttrib(m_window, GLFW_ICONIFIED);
}

vk::UniqueSurfaceKHR Window::createSurface(const vk::Instance& instance) const
{
    VkSurfaceKHR surface;

    const auto result = glfwCreateWindowSurface(instance, m_window, nullptr, &surface);
    if(result != VK_SUCCESS) {
        RAYGUN_FATAL("Unable to create window surface: {}", glfwGetError(nullptr));
    }

    return gpu::wrapUnique<vk::SurfaceKHR>(surface, instance);
}

void Window::handleEvents()
{
    if(glfwWindowShouldClose(m_window)) {
        RG().quit();
    }

    if(m_resized && !minimized()) {
        m_resized = false;

        RAYGUN_DEBUG("Resizing");

        RG().scene().camera->updateAspectRatio();
        RG().vc().resize();
        RG().renderSystem().resize();
    }
}

} // namespace raygun
