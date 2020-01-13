#pragma once

#include "raygun/scene.hpp"
#include "raygun/ui/ui.hpp"

#include "ball.hpp"

class ExampleScene : public raygun::Scene {
  public:
    ExampleScene();

    void processInput(raygun::input::Input input, double timeDelta) override;
    void update(double timeDelta) override;

  private:
    static constexpr raygun::vec3 CAMERA_OFFSET = {5.0f, 10.0f, 10.0f};

    std::shared_ptr<Ball> m_ball;

    std::unique_ptr<raygun::ui::Factory> m_uiFactory;
    std::shared_ptr<raygun::ui::Window> m_menu;

    void showMenu();
};
