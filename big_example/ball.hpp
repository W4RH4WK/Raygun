#pragma once

#include "raygun/entity.hpp"

class Ball : public raygun::Entity {
  public:
    Ball();

    void update();

  private:
    static constexpr double BUMP_SOUND_VELOCITY_CHANGE_THRESHOLD = 0.5f;

    raygun::vec3 m_previousVelocity = raygun::zero();
    std::shared_ptr<raygun::audio::Sound> m_bumpSound;
};
