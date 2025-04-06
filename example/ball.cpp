#include "ball.hpp"

#include "raygun/assert.hpp"
#include "raygun/raygun.hpp"

using namespace raygun;
using namespace raygun::physics;
using namespace physx;

Ball::Ball() : Entity("Ball", RG().resourceManager().entityLoadPath("ball")), m_bumpSound(RG().resourceManager().loadSound("bonk"))
{
    RAYGUN_INFO("Ball: Creating ball entity");
    
    // On import, all entities contained within the loaded file are attached to
    // this entity as children. However, this entity represents just the ball,
    // with a model and a physics actor.
    //
    // We therefore grab the model from our child as it is used for
    // instantiating the physics actor.

    RAYGUN_INFO("Ball: Children count: {}", children().size());
    if (children().empty()) {
        RAYGUN_ERROR("Ball: No children found in loaded entity");
    } else {
        RAYGUN_INFO("Ball: Getting model from first child");
        model = std::move(children().at(0)->model);
        RAYGUN_INFO("Ball: Clearing children");
        clearChildren();
    }

    RAYGUN_INFO("Ball: Attaching rigid dynamic physics");
    RG().physicsSystem().attachRigidDynamic(*this, false, GeometryType::Sphere);

    // our default physics actor is not enough, we also need to adjust its mass.
    RAYGUN_INFO("Ball: Setting mass properties");
    
    // Instead of using dynamic_cast which can cause issues with ABI compatibility,
    // we'll check if the actor is dynamic using PhysX's built-in methods
    auto* actor = physicsActor.get();
    if (!actor) {
        RAYGUN_ERROR("Ball: Physics actor is null");
        return;
    }
    
    // Check if the actor is a rigid dynamic actor (safer than dynamic_cast)
    if (actor->is<PxRigidDynamic>()) {
        auto* rigidBody = static_cast<PxRigidDynamic*>(actor);
        RAYGUN_INFO("Ball: Updating mass and inertia");
        PxRigidBodyExt::updateMassAndInertia(*rigidBody, 50.0f);
    } else {
        RAYGUN_ERROR("Ball: Physics actor is not a PxRigidDynamic");
    }
    
    RAYGUN_INFO("Ball: Construction complete");
}

void Ball::update()
{
    // A bump sound effect is played on abrupt velocity changes.
    //
    // We do this here for simplicity. One could also grab the contact
    // information from the physics engine.

    auto* actor = physicsActor.get();
    if (!actor || !actor->is<PxRigidDynamic>()) {
        RAYGUN_ERROR("Ball: Invalid physics actor in update");
        return;
    }
    
    auto* rigidBody = static_cast<PxRigidDynamic*>(actor);
    
    const auto velocity = toVec3(rigidBody->getLinearVelocity());
    const auto change = glm::length(m_previousVelocity - velocity);

    if(change >= BUMP_SOUND_VELOCITY_CHANGE_THRESHOLD) {
        const auto gain = std::clamp(change / 10.0, 0.1, 1.0);
        RG().audioSystem().playSoundEffect(m_bumpSound, gain, transform().position);
    }

    m_previousVelocity = velocity;
}
