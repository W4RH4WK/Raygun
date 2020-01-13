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

#include "raygun/physics/physics_system.hpp"

#include "raygun/logging.hpp"
#include "raygun/physics/physics_utils.hpp"
#include "raygun/raygun.hpp"

using namespace physx;

namespace raygun::physics {

PhysicsSystem::PhysicsSystem()
    : m_foundation(PxCreateFoundation(PX_PHYSICS_VERSION, m_allocator, m_errorCallback))
    , m_pvdTransport(PxDefaultPvdSocketTransportCreate("localhost", 5425, 10))
    , m_pvd(PxCreatePvd(*m_foundation))
    , m_physics(PxCreatePhysics(PX_PHYSICS_VERSION, *m_foundation, PxTolerancesScale(), true, m_pvd.get()))
    , m_dispatcher(PxDefaultCpuDispatcherCreate(THREADS))
    , m_cooking(PxCreateCooking(PX_PHYSICS_VERSION, *m_foundation, PxCookingParams(PxTolerancesScale())))
    , m_defaultMaterial(m_physics->createMaterial(0.8f, 0.8f, 0.6f))
{
#ifdef _DEBUG
    if(m_pvd->connect(*m_pvdTransport, PxPvdInstrumentationFlag::eALL)) {
        RAYGUN_DEBUG("Connected to PhysX debugger");
    }
    else {
        RAYGUN_DEBUG("Unable to connect to PhysX debugger");
    }
#endif

    RAYGUN_INFO("Physics system initialized");
}

namespace {

    PxFilterFlags filterShader(PxFilterObjectAttributes, PxFilterData, PxFilterObjectAttributes, PxFilterData, PxPairFlags& pairFlags, const void*, PxU32)
    {
        pairFlags = PxPairFlag::eCONTACT_DEFAULT | PxPairFlag::eNOTIFY_TOUCH_FOUND | PxPairFlag::eNOTIFY_TOUCH_PERSISTS | PxPairFlag::eNOTIFY_TOUCH_LOST
                    | PxPairFlag::eNOTIFY_CONTACT_POINTS;
        return PxFilterFlag::eDEFAULT;
    }

} // namespace

UniqueScene PhysicsSystem::createScene()
{
    PxSceneDesc desc(m_physics->getTolerancesScale());
    desc.gravity = {0.0f, -9.81f, 0.0f};
    desc.cpuDispatcher = m_dispatcher.get();
    desc.filterShader = filterShader;
    desc.flags |= PxSceneFlag::eENABLE_ENHANCED_DETERMINISM;

    const auto scene = m_physics->createScene(desc);

#ifdef _DEBUG
    if(const auto pvdClient = scene->getScenePvdClient()) {
        pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
        pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
        pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
    }
#endif

    m_simCallback = std::make_unique<SimCallback>();
    scene->setSimulationEventCallback(&*m_simCallback);

    return wrapUnique(scene);
}

namespace {
    void attachShape(PxRigidActor& actor, const Entity& entity, bool isTrigger, GeometryType geometryType, const PxMaterial& material)
    {
        auto flags = PxShapeFlag::eVISUALIZATION | PxShapeFlag::eSCENE_QUERY_SHAPE | PxShapeFlag::eSIMULATION_SHAPE;
        if(isTrigger) {
            flags = PxShapeFlag::eTRIGGER_SHAPE;
        }

        switch(geometryType) {
        case GeometryType::BoundingBox: {
            const auto [lower, upper] = entity.model->mesh->bounds();
            PxBoxGeometry geometry(toVec3(entity.transform().scaling * (upper - lower) / 2.0f));
            PxRigidActorExt::createExclusiveShape(actor, geometry, material, flags);
            break;
        }
        case GeometryType::Sphere: {
            const auto width = entity.model->mesh->width();
            PxRigidActorExt::createExclusiveShape(actor, PxSphereGeometry(width / 2.0f), material, flags);
            break;
        }
        case GeometryType::Plane: {
            auto shape = PxRigidActorExt::createExclusiveShape(actor, PxPlaneGeometry(), material, flags);
            shape->setLocalPose(PxTransformFromPlaneEquation(PxPlane(toVec3(entity.model->mesh->vertices[0].normal), 0)));
            break;
        }
        case GeometryType::ConvexMesh: {
            const auto convexMesh = RG().physicsSystem().createConvexMesh(*entity.model->mesh);
            PxConvexMeshGeometry geometry(convexMesh.get(), PxMeshScale(toVec3(entity.transform().scaling)));
            PxRigidActorExt::createExclusiveShape(actor, geometry, material, flags);
            break;
        }
        case GeometryType::TriangleMesh: {
            const auto triangleMesh = RG().physicsSystem().createTriangleMesh(*entity.model->mesh);
            const auto materials = collectPhysicsMaterials(entity.model->materials);
            PxTriangleMeshGeometry geometry(triangleMesh.get(), PxMeshScale(toVec3(entity.transform().scaling)));
            PxRigidActorExt::createExclusiveShape(actor, geometry, materials.data(), (PxU16)materials.size(), flags);
            break;
        }
        }
    }
} // namespace

void PhysicsSystem::attachRigidStatic(Entity& entity, GeometryType geometryType, PxMaterial* material)
{
    if(!material) material = m_defaultMaterial.get();

    auto actor = wrapUnique(m_physics->createRigidStatic(toTransform(entity.transform())));
    actor->setName(entity.name.c_str());
    actor->userData = (void*)&entity;

    attachShape(*actor, entity, false, geometryType, *material);

    entity.physicsActor = std::move(actor);
}

void PhysicsSystem::attachRigidDynamic(Entity& entity, bool isKinematic, GeometryType geometryType, PxMaterial* material)
{
    if(!material) material = m_defaultMaterial.get();

    auto actor = wrapUnique(m_physics->createRigidDynamic(toTransform(entity.transform())));
    actor->setName(entity.name.c_str());
    actor->userData = (void*)&entity;

    actor->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, isKinematic);

    attachShape(*actor, entity, false, geometryType, *material);

    entity.physicsActor = std::move(actor);
}

void PhysicsSystem::makeTrigger(Entity& entity, TriggerCallback callback, GeometryType geometryType)
{
    auto actor = wrapUnique(m_physics->createRigidStatic(toTransform(entity.transform())));
    actor->setName(entity.name.c_str());

    attachShape(*actor, entity, true, geometryType, *m_defaultMaterial);

    addTriggerEvent(actor.get(), callback);

    entity.physicsActor = std::move(actor);
    entity.model.reset();
}

UniqueTriangleMesh PhysicsSystem::createTriangleMesh(const render::Mesh& mesh)
{
    const auto materialIndices = getMaterialIndices(mesh);

    PxTriangleMeshDesc meshDesc = {};
    meshDesc.points.count = (PxU32)mesh.vertices.size();
    meshDesc.points.data = mesh.vertices.data();
    meshDesc.points.stride = sizeof(mesh.vertices[0]);
    meshDesc.triangles.count = (PxU32)(mesh.numFaces());
    meshDesc.triangles.data = mesh.indices.data();
    meshDesc.triangles.stride = 3 * sizeof(PxU32);
    meshDesc.materialIndices.data = materialIndices.data();
    meshDesc.materialIndices.stride = sizeof(materialIndices[0]);

    auto params = m_cooking->getParams();
    params.midphaseDesc = PxMeshMidPhase::eBVH34;
    params.midphaseDesc.mBVH34Desc.numPrimsPerLeaf = 4;
    params.suppressTriangleMeshRemapTable = true;
    params.meshPreprocessParams |= PxMeshPreprocessingFlag::eWELD_VERTICES;
    params.meshWeldTolerance = 0.05f;
    m_cooking->setParams(params);

    return wrapUnique(m_cooking->createTriangleMesh(meshDesc, m_physics->getPhysicsInsertionCallback()));
}

UniqueConvexMesh PhysicsSystem::createConvexMesh(const render::Mesh& mesh)
{
    PxConvexMeshDesc desc = {};
    desc.points.count = (PxU32)mesh.vertices.size();
    desc.points.data = mesh.vertices.data();
    desc.points.stride = sizeof(mesh.vertices[0]);
    desc.flags = PxConvexFlag::eCOMPUTE_CONVEX;

    auto params = m_cooking->getParams();
    params.convexMeshCookingType = PxConvexMeshCookingType::eQUICKHULL;
    params.gaussMapLimit = 16;
    m_cooking->setParams(params);

    return wrapUnique(m_cooking->createConvexMesh(desc, m_physics->getPhysicsInsertionCallback()));
}

void PhysicsSystem::simulate(PxScene& scene, float timeDelta)
{
    if(m_paused) return;

    scene.simulate(timeDelta);
    scene.fetchResults(true);
}

void PhysicsSystem::addTriggerEvent(const PxActor* trigger, TriggerCallback handler)
{
    m_simCallback->addTriggerEvent(trigger, handler);
}

void PhysicsSystem::clearTriggerEvents()
{
    m_simCallback->clearTriggerEvents();
}

void PhysicsSystem::addContactEvent(const PxActor* trigger, ContactCallback handler)
{
    m_simCallback->addContactEvent(trigger, handler);
}

void PhysicsSystem::clearContactEvents()
{
    m_simCallback->clearContactEvents();
}

void PhysicsSystem::update(double timeDelta)
{
    auto& scene = RG().scene();

    connectActorsToScene(scene);

    simulate(*scene.pxScene, (float)timeDelta);

    // Update transforms
    scene.root->forEachEntity([&](Entity& entity) {
        if(!entity.physicsActor) return;

        if(auto rigidDynamic = dynamic_cast<PxRigidDynamic*>(entity.physicsActor.get())) {
            auto transform = physics::toTransform(rigidDynamic->getGlobalPose(), entity.transform().scaling);
            entity.setTransform(entity.parentTransform().inverse() * transform);
        }
    });
}

void PhysicsSystem::connectActorsToScene(Scene& scene)
{
    auto actors = getActors(*scene.pxScene);

    // Ensure all entities with physics actors are connected with the physics
    // scene.
    scene.root->forEachEntity([&](Entity& entity) {
        if(!entity.physicsActor) return;

        const auto it = actors.find(entity.physicsActor.get());
        if(it == actors.end()) {
            scene.pxScene->addActor(*entity.physicsActor);
        }
        else {
            actors.erase(it);
        }
    });

    // Remove obsolete physics actors from physics scene.
    for(const auto actor: actors) {
        scene.pxScene->removeActor(*actor);
    }
}

} // namespace raygun::physics
