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

#pragma once

#include "raygun/entity.hpp"
#include "raygun/material.hpp"

namespace raygun::physics {

/// Function to be executed when triggered. Return value indicates whether
/// further callbacks should be executed.
using TriggerCallback = std::function<bool(physx::PxTriggerPair)>;

enum class Touch { Found, Persist, Lost };

/// Function to be executed when contact is found or lost.
using ContactCallback = std::function<void(Touch touch, Entity& other, raygun::Material* otherMaterial)>;

class SimCallback : public physx::PxSimulationEventCallback {
  public:
    virtual void onTrigger(physx::PxTriggerPair* pairs, physx::PxU32 count) override;

    virtual void onConstraintBreak(physx::PxConstraintInfo*, physx::PxU32) override {}

    virtual void onWake(physx::PxActor**, physx::PxU32) override {}

    virtual void onSleep(physx::PxActor**, physx::PxU32) override {}

    virtual void onContact(const physx::PxContactPairHeader&, const physx::PxContactPair*, physx::PxU32) override;

    virtual void onAdvance(const physx::PxRigidBody* const*, const physx::PxTransform*, const physx::PxU32) override {}

    void addTriggerEvent(const physx::PxActor* trigger, TriggerCallback handler);

    void clearTriggerEvents();

    void addContactEvent(const physx::PxActor* trigger, ContactCallback handler);

    void clearContactEvents();

  private:
    std::map<const physx::PxActor*, TriggerCallback> triggerEvents;

    std::map<const physx::PxActor*, ContactCallback> contactEvents;
};
} // namespace raygun::physics
