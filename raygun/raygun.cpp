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

#include "raygun/raygun.hpp"

#include "raygun/assert.hpp"
#include "raygun/info.hpp"
#include "raygun/logging.hpp"
#include "raygun/ui/ui.hpp"

namespace raygun {

static Raygun* instance;

Raygun::Raygun(string_view title, UniqueConfig config)
{
    if(instance) {
        RAYGUN_FATAL(RAYGUN_NAME " instance already initialized");
    }
    else {
        instance = this;
    }

    if(config) {
        m_config = std::move(config);
    }
    else {
        m_config = std::make_unique<Config>(configDirectory() / "config.json");
    }

    m_resourceManager = std::make_unique<ResourceManager>();

    m_glfwRuntime = std::make_unique<glfw::Runtime>();

    m_window = std::make_unique<Window>(title);

    m_inputSystem = std::make_unique<input::InputSystem>();

    m_vc = std::make_unique<VulkanContext>();

    m_profiler = std::make_unique<Profiler>();

    m_computeSystem = std::make_unique<compute::ComputeSystem>();

    m_renderSystem = std::make_unique<render::RenderSystem>();

    m_physicsSystem = std::make_unique<physics::PhysicsSystem>();

    m_audioSystem = std::make_unique<audio::AudioSystem>();
    m_audioSystem->setupDefaultSources();

    loadScene(std::make_unique<Scene>());

    RAYGUN_INFO(RAYGUN_NAME " initialized");
}

Raygun::~Raygun()
{
    // Ensure GPU pipeline is empty. Otherwise objects may be destroyed while
    // in use.
    m_vc->waitIdle();

    instance = nullptr;
}

void Raygun::loadScene(UniqueScene scene)
{
    RAYGUN_ASSERT(scene);
    m_nextScene = std::move(scene);
}

void Raygun::loop()
{
    RAYGUN_INFO("Begin main loop");

    while(!m_shouldQuit) {
        m_glfwRuntime->pollEvents();

        m_window->handleEvents();

        if(m_window->minimized()) continue;

        const auto input = m_inputSystem->handleEvents();

        const auto timeDelta = updateTimestamp();

        m_profiler->startFrame();

        if(m_nextScene) {
            finalizeLoadScene();
        }

        m_renderSystem->preSimulation();

        m_scene->preSimulation();

        m_animationSystem->update(timeDelta);

        m_physicsSystem->update(timeDelta);

        if(!ui::runUI(*m_scene->root, timeDelta, input)) {
            m_scene->processInput(input, timeDelta);
        }

        m_scene->root->forEachEntity([timeDelta](auto& ent) {
            if(auto animEnt = dynamic_cast<AnimatableEntity*>(&ent)) {
                animEnt->update(timeDelta);
            }
        });

        m_scene->update(timeDelta);

        m_audioSystem->update();

        m_renderSystem->render(*m_scene);
    }

    RAYGUN_INFO("End main loop");
}

void Raygun::quit()
{
    RAYGUN_INFO("Quitting");

    m_shouldQuit = true;
}

Config& Raygun::config()
{
    if(!m_config) {
        RAYGUN_FATAL("Config not set");
    }

    return *m_config;
}

glfw::Runtime& Raygun::glfwRuntime()
{
    if(!m_glfwRuntime) {
        RAYGUN_FATAL("GLFW Runtime not set");
    }

    return *m_glfwRuntime;
}

Window& Raygun::window()
{
    if(!m_window) {
        RAYGUN_FATAL("Window not set");
    }

    return *m_window;
}

input::InputSystem& Raygun::inputSystem()
{
    if(!m_inputSystem) {
        RAYGUN_FATAL("Input system not set");
    }

    return *m_inputSystem;
}

VulkanContext& Raygun::vc()
{
    if(!m_vc) {
        RAYGUN_FATAL("Vulkan Context not set");
    }

    return *m_vc;
}

Profiler& Raygun::profiler()
{
    if(!m_profiler) {
        RAYGUN_FATAL("Profiler not set");
    }

    return *m_profiler;
}

compute::ComputeSystem& Raygun::computeSystem()
{
    if(!m_computeSystem) {
        RAYGUN_FATAL("Compute system not set");
    }

    return *m_computeSystem;
}

render::RenderSystem& Raygun::renderSystem()
{
    if(!m_renderSystem) {
        RAYGUN_FATAL("Render system not set");
    }

    return *m_renderSystem;
}

animation::AnimationSystem& Raygun::animationSystem()
{
    if(!m_animationSystem) {
        RAYGUN_FATAL("Animation system not set");
    }

    return *m_animationSystem;
}

physics::PhysicsSystem& Raygun::physicsSystem()
{
    if(!m_physicsSystem) {
        RAYGUN_FATAL("Physics system not set");
    }

    return *m_physicsSystem;
}

audio::AudioSystem& Raygun::audioSystem()
{
    if(!m_audioSystem) {
        RAYGUN_FATAL("Audio system not set");
    }

    return *m_audioSystem;
}

ResourceManager& Raygun::resourceManager()
{
    if(!m_resourceManager) {
        RAYGUN_FATAL("Resource manager not set");
    }

    return *m_resourceManager;
}

Scene& Raygun::scene()
{
    if(!m_scene) {
        RAYGUN_FATAL("Scene not set");
    }

    return *m_scene;
}

double Raygun::time()
{
    return std::chrono::duration<double>(m_time).count();
}

double Raygun::updateTimestamp()
{
    using namespace std::chrono_literals;

    const auto now = Clock::now();

    auto delta = now - m_timestamp;
    m_timestamp = now;

    // Large time deltas can cause issues with physics simulations. Capping it
    // here affects the whole simulation (not just physics) equally.
    delta = std::min<Clock::duration>(delta, 50ms);

    m_time += delta;

    return std::chrono::duration<double>(delta).count();
}

void Raygun::finalizeLoadScene()
{
    RAYGUN_INFO("Loading scene");

    m_vc->device->waitIdle();

    std::swap(m_scene, m_nextScene);
    m_nextScene.reset();

    m_resourceManager->clearUnusedModelsAndMaterials();

    m_renderSystem->resetUniformBuffer();

    m_renderSystem->setupModelBuffers();
    m_renderSystem->raytracer().setupBottomLevelAS();

    m_timestamp = Clock::now();

    m_scene->camera->updateProjection();
}

Raygun& RG()
{
    if(!instance) {
        RAYGUN_FATAL(RAYGUN_NAME " instance not set");
    }

    return *instance;
}

} // namespace raygun
