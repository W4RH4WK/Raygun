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

#include "raygun/render/mesh.hpp"
#include "raygun/transform.hpp"

namespace raygun::physics {

/// Resource cleanup functor, used with std::unique_ptr.
template<typename T>
struct Deleter {
    void operator()(T* p)
    {
        if(p) p->release();
    }
};

// Using the same deleter for the whole hierarchy to enable unique pointer
// conversions.
template<typename T>
struct deleter_type {
    using type = std::conditional_t<std::is_base_of_v<physx::PxBase, T>, physx::PxBase, T>;
};
template<typename T>
using deleter_type_t = typename deleter_type<T>::type;

template<typename T, typename D = Deleter<deleter_type_t<T>>>
using UniqueHandle = std::unique_ptr<T, D>;

using UniqueFoundation = UniqueHandle<physx::PxFoundation>;
using UniquePvd = UniqueHandle<physx::PxPvd>;
using UniquePvdTransport = UniqueHandle<physx::PxPvdTransport>;
using UniquePhysics = UniqueHandle<physx::PxPhysics>;
using UniqueDefaultCpuDispatcher = UniqueHandle<physx::PxDefaultCpuDispatcher>;
using UniqueScene = UniqueHandle<physx::PxScene>;
using UniqueShape = UniqueHandle<physx::PxShape>;
using UniqueMaterial = UniqueHandle<physx::PxMaterial>;
using UniqueCooking = UniqueHandle<physx::PxCooking>;
using UniqueTriangleMesh = UniqueHandle<physx::PxTriangleMesh>;
using UniqueConvexMesh = UniqueHandle<physx::PxConvexMesh>;
using UniqueActor = UniqueHandle<physx::PxActor>;
using UniqueRigidActor = UniqueHandle<physx::PxRigidActor>;
using UniqueRigidDynamic = UniqueHandle<physx::PxRigidDynamic>;
using UniqueRigidStatic = UniqueHandle<physx::PxRigidStatic>;

template<typename T, typename D = Deleter<deleter_type_t<T>>>
UniqueHandle<T, D> wrapUnique(T* p)
{
    return UniqueHandle<T, D>(p);
}

//////////////////////////////////////////////////////////////////////////

/// Convert Raygun Vec3 to PhysX Vec3.
static inline const physx::PxVec3& toVec3(const vec3& v)
{
    return reinterpret_cast<const physx::PxVec3&>(v);
}

/// Convert PhysX Vec3 to Raygun Vec3.
static inline const vec3& toVec3(const physx::PxVec3& v)
{
    return reinterpret_cast<const vec3&>(v);
}

/// Convert Raygun Quaternion to PhysX Quaternion.
static inline const physx::PxQuat& toQuat(const quat& q)
{
    return reinterpret_cast<const physx::PxQuat&>(q);
}

/// Convert PhysX Quaternion to Raygun Quaternion.
static inline const quat& toQuat(const physx::PxQuat& q)
{
    return reinterpret_cast<const quat&>(q);
}

/// Convert Raygun transform to PhysX transform.
static inline physx::PxTransform toTransform(const Transform& t)
{
    return physx::PxTransform{toVec3(t.position), toQuat(t.rotation)};
}

/// Convert PhysX transform to Raygun transform.
static inline Transform toTransform(const physx::PxTransform& t, const vec3& scaling)
{
    Transform result;
    result.position = toVec3(t.p);
    result.rotation = toQuat(t.q);
    result.scaling = scaling;
    return result;
}

static inline std::set<physx::PxActor*> getActors(physx::PxScene& scene)
{
    const auto typeFlags = physx::PxActorTypeFlag::eRIGID_DYNAMIC | physx::PxActorTypeFlag::eRIGID_STATIC;

    const auto count = scene.getNbActors(typeFlags);

    std::vector<physx::PxActor*> actors(count);
    scene.getActors(typeFlags, actors.data(), (physx::PxU32)actors.size());

    return {actors.begin(), actors.end()};
}

static inline UniqueMaterial cloneMaterial(physx::PxPhysics& physics, const physx::PxMaterial& material)
{
    const auto staticFriction = material.getStaticFriction();
    const auto dynamicFriction = material.getStaticFriction();
    const auto restitution = material.getRestitution();

    auto result = wrapUnique(physics.createMaterial(staticFriction, dynamicFriction, restitution));
    result->setBaseFlags(material.getBaseFlags());
    result->setFlags(material.getFlags());

    return result;
}

static inline std::vector<physx::PxMaterialTableIndex> getMaterialIndices(const render::Mesh& mesh)
{
    std::vector<physx::PxMaterialTableIndex> result;
    result.reserve(mesh.numFaces());
    mesh.forEachFace([&](const render::Vertex& v0, auto, auto) { result.push_back((physx::PxMaterialTableIndex)v0.matIndex); });
    return result;
}

} // namespace raygun::physics
