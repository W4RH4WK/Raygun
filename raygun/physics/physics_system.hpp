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

#include "raygun/entity.hpp"
#include "raygun/physics/physics_error_callback.hpp"
#include "raygun/physics/physics_sim_callback.hpp"
#include "raygun/physics/physics_utils.hpp"
#include "raygun/render/mesh.hpp"
#include "raygun/scene.hpp"

namespace raygun::physics {

enum class GeometryType {
    BoundingBox,
    Sphere,
    Plane,
    ConvexMesh,
    TriangleMesh,
};

class PhysicsSystem {
  public:
    PhysicsSystem();

    UniqueScene createScene();

    void attachRigidStatic(Entity& entity, GeometryType geometryType, physx::PxMaterial* material = nullptr);

    void attachRigidDynamic(Entity& entity, bool isKinematic, GeometryType geometryType, physx::PxMaterial* material = nullptr);

    /// Turns the given entity into a trigger (using its location and geometry)
    /// with the given callback. Removes the entity's attached model.
    void makeTrigger(Entity& entity, TriggerCallback callback, GeometryType geometryType);

    UniqueTriangleMesh createTriangleMesh(const render::Mesh& mesh);

    UniqueConvexMesh createConvexMesh(const render::Mesh& mesh);

    void simulate(physx::PxScene& scene, float timeDelta);

    physx::PxPhysics& physics() { return *m_physics; }

    physx::PxCooking& cooking() { return *m_cooking; }

    void addTriggerEvent(const physx::PxActor* trigger, TriggerCallback handler);
    void clearTriggerEvents();

    void addContactEvent(const physx::PxActor* trigger, ContactCallback handler);
    void clearContactEvents();

    void update(double timeDelta);

    void pause() { m_paused = true; }
    void unpause() { m_paused = false; }

  private:
    static constexpr physx::PxU32 THREADS = 2;

    physx::PxDefaultAllocator m_allocator;

    ErrorCallback m_errorCallback;

    UniqueFoundation m_foundation;

    UniquePvdTransport m_pvdTransport;
    UniquePvd m_pvd;

    UniquePhysics m_physics;

    UniqueDefaultCpuDispatcher m_dispatcher;

    UniqueCooking m_cooking;

    UniqueMaterial m_defaultMaterial;

    std::unique_ptr<SimCallback> m_simCallback;

    bool m_paused = false;

    void connectActorsToScene(Scene& scene);
};

using UniquePhysicsSystem = std::unique_ptr<PhysicsSystem>;

} // namespace raygun::physics
