#include "example_scene.hpp"

#include "raygun/animation/animation.hpp"
#include "raygun/animation/animator.hpp"
#include "raygun/assert.hpp"
#include "raygun/raygun.hpp"

using namespace raygun;
using namespace raygun::physics;

ExampleScene::ExampleScene()
{
    // setup level
    auto level = RG().resourceManager().loadEntity("room");
    level->forEachEntity([](Entity& entity) {
        if(entity.model) {
            RG().physicsSystem().attachRigidStatic(entity, GeometryType::TriangleMesh);
        }
    });
    root->addChild(level);

    // setup ball
    m_ball = std::make_shared<Ball>();
    m_ball->moveTo({3.0f, 0.0f, -3.0f});
    root->addChild(m_ball);

    // setup music
    auto musicTrack = RG().resourceManager().loadSound("lone_rider");
    RG().audioSystem().music().play(musicTrack);

    // setup ui stuff
    const auto font = RG().resourceManager().loadFont("NotoSans");
    m_uiFactory = std::make_unique<ui::Factory>(font);
}

void ExampleScene::processInput(raygun::input::Input input, double timeDelta)
{
    if(input.reload) {
        RG().loadScene(std::make_unique<ExampleScene>());
    }

    if(input.cancel) {
        showMenu();
    }

    auto inputDir = vec2{-input.dir.y, -input.dir.x};

    // Take camera direction into account.
    const auto cam2D = glm::normalize(vec2(CAMERA_OFFSET.x, CAMERA_OFFSET.z));
    const auto angle = glm::orientedAngle(vec2(0, 1), cam2D);
    inputDir = glm::rotate(inputDir, angle);

    const auto strength = 2000.0 * timeDelta;

    auto rigidDynamic = dynamic_cast<physx::PxRigidDynamic*>(m_ball->physicsActor.get());
    RAYGUN_ASSERT(rigidDynamic);
    rigidDynamic->addTorque((float)strength * physx::PxVec3(inputDir.x, 0.f, inputDir.y), physx::PxForceMode::eIMPULSE);
}

void ExampleScene::update(double)
{
    camera->moveTo(m_ball->transform().position + CAMERA_OFFSET);
    camera->lookAt(m_ball->transform().position);

    m_ball->update();
}

void ExampleScene::showMenu()
{
    m_menu = m_uiFactory->window("menu", "Menu");
    m_uiFactory->addWithLayout(*m_menu, ui::Layout(vec2(0.5, 0.2), vec2(0, 0.3)), [&](ui::Factory& f) {
        f.button("Continue", [&] { camera->removeChild(m_menu); });
        f.button("Quit", [] { RG().renderSystem().makeFade<render::FadeTransition>(0.4, []() { RG().quit(); }); });
    });
    m_menu->doLayout();
    m_menu->move(vec3{0.0f, 0.0f, -4.0f});
    m_menu->animator = std::make_unique<animation::TransformAnimator>();
    m_menu->animator->animation = animation::scaleAnimation(vec3(1, 0, 1), vec3(1), 0.25);

    camera->addChild(m_menu);

    // Alternatively, you can spawn the test window to see all available
    // controls and layouts. Note that this window cannot be closed as no button
    // has an action associated with it.

    // camera->addChild(ui::uiTestWindow(*m_uiFactory));
}
