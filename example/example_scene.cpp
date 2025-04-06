#include "example_scene.hpp"

#include "raygun/assert.hpp"
#include "raygun/raygun.hpp"

using namespace raygun;
using namespace raygun::physics;

ExampleScene::ExampleScene()
{
    RAYGUN_INFO("ExampleScene: Creating scene");
    
    // setup level
    RAYGUN_INFO("ExampleScene: Loading level entity 'room'");
    auto level = RG().resourceManager().loadEntity("room");
    
    RAYGUN_INFO("ExampleScene: Setting up physics for level objects");
    level->forEachEntity([](Entity& entity) {
        if(entity.model) {
            RAYGUN_INFO("ExampleScene: Attaching rigid static to entity: {}", entity.name);
            RG().physicsSystem().attachRigidStatic(entity, GeometryType::TriangleMesh);
        }
    });
    
    RAYGUN_INFO("ExampleScene: Adding level to scene root");
    root->addChild(level);

    // setup ball
    RAYGUN_INFO("ExampleScene: Creating ball entity");
    m_ball = std::make_shared<Ball>();
    m_ball->moveTo({3.0f, 0.0f, -3.0f});
    
    RAYGUN_INFO("ExampleScene: Adding ball to scene root");
    root->addChild(m_ball);

    // setup music
    RAYGUN_INFO("ExampleScene: Loading music track 'lone_rider'");
    auto musicTrack = RG().resourceManager().loadSound("lone_rider");
    
    RAYGUN_INFO("ExampleScene: Starting music playback");
    RG().audioSystem().music().play(musicTrack);

    // setup ui stuff
    RAYGUN_INFO("ExampleScene: Loading font 'NotoSans'");
    const auto font = RG().resourceManager().loadFont("NotoSans");
    
    RAYGUN_INFO("ExampleScene: Creating UI factory");
    m_uiFactory = std::make_unique<ui::Factory>(font);
    
    RAYGUN_INFO("ExampleScene: Construction complete");
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

    auto* actor = m_ball->physicsActor.get();
    if (!actor || !actor->is<physx::PxRigidDynamic>()) {
        RAYGUN_ERROR("ExampleScene: Invalid ball physics actor");
        return;
    }
    
    auto* rigidDynamic = static_cast<physx::PxRigidDynamic*>(actor);
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
    m_menu->setAnimation(ScaleAnimation(0.25, vec3(1, 0, 1), vec3(1)));

    camera->addChild(m_menu);

    // Alternatively, you can spawn the test window to see all available
    // controls and layouts. Note that this window cannot be closed as no button
    // has an action associated with it.

    // camera->addChild(ui::uiTestWindow(*m_uiFactory));
}
