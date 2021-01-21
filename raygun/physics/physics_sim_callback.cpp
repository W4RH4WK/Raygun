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

#include "raygun/physics/physics_sim_callback.hpp"

#include "raygun/assert.hpp"
#include "raygun/logging.hpp"
#include "raygun/material.hpp"

using namespace physx;

namespace raygun::physics {

void SimCallback::onTrigger(PxTriggerPair* pairs, PxU32 count)
{
    for(unsigned i = 0; i < count; ++i) {
        auto p = pairs[i];
        auto handler = triggerEvents.find(p.triggerActor);
        if(handler != triggerEvents.end()) {
            if(!handler->second(p)) {
                break;
            }
        }
        else {
            RAYGUN_TRACE("Unhandled trigger");
        }
    }
}

void SimCallback::onContact(const PxContactPairHeader& header, const PxContactPair* pairs, [[maybe_unused]] PxU32 count)
{
    RAYGUN_ASSERT(count > 0);
    const auto& pair = pairs[0];

    auto touch = Touch::Found;
    if(pair.events & PxPairFlag::eNOTIFY_TOUCH_PERSISTS) {
        touch = Touch::Persist;
    }
    else if(pair.events & PxPairFlag::eNOTIFY_TOUCH_LOST) {
        touch = Touch::Lost;
    }

    PxContactPairPoint contact = {};
    pair.extractContacts(&contact, 1);

    if(auto it = contactEvents.find(header.actors[0]); it != contactEvents.end()) {
        auto& other = *static_cast<Entity*>(header.actors[1]->userData);

        Material* material = nullptr;
        if(touch != Touch::Lost && contact.internalFaceIndex1 != PXC_CONTACT_NO_FACE_INDEX) {
            material = static_cast<Material*>(pair.shapes[1]->getMaterialFromInternalFaceIndex(contact.internalFaceIndex1)->userData);
        }

        it->second(touch, other, material);
    }

    if(auto it = contactEvents.find(header.actors[1]); it != contactEvents.end()) {
        auto& other = *static_cast<Entity*>(header.actors[0]->userData);

        Material* material = nullptr;
        if(touch != Touch::Lost && contact.internalFaceIndex0 != PXC_CONTACT_NO_FACE_INDEX) {
            material = static_cast<Material*>(pair.shapes[0]->getMaterialFromInternalFaceIndex(contact.internalFaceIndex0)->userData);
        }

        it->second(touch, other, material);
    }
}

void SimCallback::addTriggerEvent(const PxActor* trigger, physics::TriggerCallback handler)
{
    triggerEvents.insert({trigger, handler});
}

void SimCallback::clearTriggerEvents()
{
    triggerEvents.clear();
}

void SimCallback::addContactEvent(const PxActor* trigger, ContactCallback handler)
{
    contactEvents.insert({trigger, handler});
}

void SimCallback::clearContactEvents()
{
    contactEvents.clear();
}

} // namespace raygun::physics
