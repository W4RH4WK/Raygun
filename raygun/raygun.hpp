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

#include "raygun/audio/audio_system.hpp"
#include "raygun/compute/compute_system.hpp"
#include "raygun/config.hpp"
#include "raygun/info.hpp"
#include "raygun/input/input_system.hpp"
#include "raygun/physics/physics_system.hpp"
#include "raygun/profiler.hpp"
#include "raygun/render/render_system.hpp"
#include "raygun/resource_manager.hpp"
#include "raygun/scene.hpp"
#include "raygun/utils/glfw_utils.hpp"
#include "raygun/vulkan_context.hpp"
#include "raygun/window.hpp"

namespace raygun {

/// This is the god class that sets up and stores the engine's components.
class Raygun {
  public:
    Raygun(string_view title = APP_TITLE, UniqueConfig config = {});
    ~Raygun();

    void loadScene(UniqueScene scene);

    /// Calling this starts the main loop of the engine. Blocks until we are
    /// done.
    void loop();

    /// Signals the engine to initiate shutdown.
    void quit();

    Config& config();

    glfw::Runtime& glfwRuntime();

    Window& window();

    input::InputSystem& inputSystem();

    VulkanContext& vc();

    Profiler& profiler();

    compute::ComputeSystem& computeSystem();

    render::RenderSystem& renderSystem();

    physics::PhysicsSystem& physicsSystem();

    audio::AudioSystem& audioSystem();

    ResourceManager& resourceManager();

    Scene& scene();

    /// Returns the active time passed since engine initialization.
    double time();

  private:
    // The order of these members is important as they dictate the sequence of
    // destruction. Think twice before changing something here.

    UniqueConfig m_config;

    glfw::UniqueRuntime m_glfwRuntime;

    UniqueWindow m_window;

    input::UniqueInputSystem m_inputSystem;

    UniqueVulkanContext m_vc;

    UniqueProfiler m_profiler;

    compute::UniqueComputeSystem m_computeSystem;

    render::UniqueRenderSystem m_renderSystem;

    physics::UniquePhysicsSystem m_physicsSystem;

    audio::UniqueAudioSystem m_audioSystem;

    UniqueResourceManager m_resourceManager;

    UniqueScene m_scene;
    UniqueScene m_nextScene;

    bool m_shouldQuit = false;

    Clock::duration m_time = Clock::duration::zero();

    Clock::time_point m_timestamp = Clock::now();

    /// Updates the internal time tracking and returns the time-delta.
    double updateTimestamp();

    void finalizeLoadScene();
};

/// Raygun singleton accessor.
Raygun& RG();

} // namespace raygun
