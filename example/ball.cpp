#include "ball.hpp"

#include "raygun/assert.hpp"
#include "raygun/raygun.hpp"

using namespace raygun;
using namespace raygun::physics;
using namespace physx;

Ball::Ball() : Entity("Ball", RG().resourceManager().entityLoadPath("ball")), m_bumpSound(RG().resourceManager().loadSound("bonk"))
{
    // On import, all entities contained within the loaded file are attached to
    // this entity as children. However, this entity represents just the ball,
    // with a model and a physics actor.
    //
    // We therefore grab the model from our child as it is used for
    // instantiating the physics actor.

    model = std::move(children().at(0)->model);
    clearChildren();

    RG().physicsSystem().attachRigidDynamic(*this, false, GeometryType::Sphere);

    // our default physics actor is not enough, we also need to adjust its mass.
    auto rigidBody = dynamic_cast<PxRigidDynamic*>(physicsActor.get());
    RAYGUN_ASSERT(rigidBody);
    PxRigidBodyExt::updateMassAndInertia(*rigidBody, 50.0f);
}

void Ball::update()
{
    // A bump sound effect is played on abrupt velocity changes.
    //
    // We do this here for simplicity. One could also grab the contact
    // information from the physics engine.

    auto rigidBody = dynamic_cast<PxRigidDynamic*>(physicsActor.get());
    RAYGUN_ASSERT(rigidBody);

    const auto velocity = toVec3(rigidBody->getLinearVelocity());
    const auto change = glm::length(m_previousVelocity - velocity);

    if(change >= BUMP_SOUND_VELOCITY_CHANGE_THRESHOLD) {
        const auto gain = std::clamp(change / 10.0, 0.1, 1.0);
        RG().audioSystem().playSoundEffect(m_bumpSound, gain, transform().position);
    }

    m_previousVelocity = velocity;
}
