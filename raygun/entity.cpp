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

#include "entity.hpp"

#include "raygun/logging.hpp"
#include "raygun/raygun.hpp"
#include "raygun/utils/assimp_utils.hpp"

namespace raygun {

namespace {
    std::shared_ptr<render::Mesh> loadMesh(const aiMesh& aimesh)
    {
        auto result = std::make_shared<render::Mesh>();

        result->vertices.reserve(aimesh.mNumVertices);
        for(auto i = 0u; i < aimesh.mNumVertices; ++i) {
            const auto& position = aimesh.mVertices[i];
            const auto& normal = aimesh.mNormals[i];

            auto& vertex = result->vertices.emplace_back();
            vertex.position = {position.x, position.y, position.z};
            vertex.normal = {normal.x, normal.y, normal.z};
            vertex.matIndex = aimesh.mMaterialIndex;
        }

        result->indices.reserve((size_t)aimesh.mNumFaces * 3);
        for(auto i = 0u; i < aimesh.mNumFaces; ++i) {
            const auto& face = aimesh.mFaces[i];

            if(face.mNumIndices != 3) {
                RAYGUN_WARN("Face {} of mesh {} has {} vertices, skipping", i, aimesh.mName.C_Str(), face.mNumIndices);
                continue;
            }

            result->indices.push_back(face.mIndices[0]);
            result->indices.push_back(face.mIndices[1]);
            result->indices.push_back(face.mIndices[2]);
        }

        RAYGUN_DEBUG("Loaded Mesh: {}: {} vertices", aimesh.mName.C_Str(), result->vertices.size());

        return result;
    }

    std::shared_ptr<render::Mesh> collapseMeshes(const aiScene* aiscene, const aiNode* ainode)
    {
        auto result = std::make_shared<render::Mesh>();

        for(auto i = 0u; i < ainode->mNumMeshes; ++i) {
            const auto mesh = loadMesh(*aiscene->mMeshes[ainode->mMeshes[i]]);
            result->merge(*mesh);
        }

        for(auto i = 0u; i < ainode->mNumChildren; ++i) {
            const auto childMesh = collapseMeshes(aiscene, ainode->mChildren[i]);
            result->merge(*childMesh);
        }

        return result;
    }
} // namespace

Entity::Entity(string_view name) : name(name) {}

Entity::Entity(string_view name, fs::path filepath, bool loadMaterials) : Entity(name)
{
    Assimp::Importer importer;
    importer.SetPropertyBool(AI_CONFIG_IMPORT_COLLADA_USE_COLLADA_NAMES, true);

    const auto aiscene = importer.ReadFile(filepath.string(), aiProcess_Triangulate);
    if(!aiscene) {
        RAYGUN_ERROR("Unable to load: {}: {}", name, importer.GetErrorString());
        return;
    }

    std::vector<std::shared_ptr<Material>> materials;
    if(loadMaterials) {
        materials.reserve(aiscene->mNumMaterials);

        for(auto i = 0u; i < aiscene->mNumMaterials; ++i) {
            const auto aimaterial = aiscene->mMaterials[i];

            aiString matName;
            aimaterial->Get(AI_MATKEY_NAME, matName);
            materials.push_back(RG().resourceManager().loadMaterial(matName.C_Str()));
        }
    }

    for(auto i = 0u; i < aiscene->mRootNode->mNumChildren; ++i) {
        const auto ainode = aiscene->mRootNode->mChildren[i];

        auto childModel = std::make_shared<render::Model>();
        childModel->mesh = collapseMeshes(aiscene, ainode);
        childModel->materials = materials;

        RG().resourceManager().registerModel(childModel);

        auto child = emplaceChild(ainode->mName.C_Str());
        child->setTransform(utils::toTransform(ainode->mTransformation));
        child->model = childModel;
    }
}

void Entity::addChild(std::shared_ptr<Entity> child)
{
    RAYGUN_ASSERT(!child->m_parent);
    child->setParent(this);

    m_children.push_back(child);
}

std::shared_ptr<raygun::Entity> Entity::emplaceChild(string_view childName)
{
    auto child = std::make_shared<Entity>(childName);
    child->setParent(this);

    return m_children.emplace_back(child);
}

void Entity::replaceChild(const std::shared_ptr<Entity>& oldChild, std::shared_ptr<Entity> newChild)
{
    RAYGUN_ASSERT(oldChild->m_parent == this);

    auto it = std::find(m_children.begin(), m_children.end(), oldChild);
    if(it == m_children.cend()) {
        RAYGUN_WARN("Supposed to replace entity {} from {}, but not found.", oldChild->name, name);
        return;
    }

    oldChild->clearParent();
    newChild->setParent(this);

    *it = newChild;
}

void Entity::removeChild(const std::shared_ptr<Entity>& child)
{
    RAYGUN_ASSERT(child->m_parent == this);

    auto it = std::find(m_children.cbegin(), m_children.cend(), child);
    if(it == m_children.cend()) {
        RAYGUN_WARN("Supposed to remove entity {} from {}, but not found.", child->name, name);
        return;
    }

    child->clearParent();

    m_children.erase(it);
}

void Entity::clearChildren()
{
    for(const auto& child: m_children) {
        child->clearParent();
    }

    m_children.clear();
}

void Entity::setTransform(Transform transform)
{
    invalidateChildrenCachedParentTransform();
    m_transform = transform;
    updatePhysicsTransform();
}

Transform Entity::parentTransform() const
{
    if(!m_cachedParentTransform) {
        m_cachedParentTransform = m_parent ? m_parent->globalTransform() : Transform{};
    }

    return *m_cachedParentTransform;
}

Transform Entity::globalTransform() const
{
    return parentTransform() * m_transform;
}

void Entity::move(const vec3& translation)
{
    invalidateChildrenCachedParentTransform();
    m_transform.move(translation);
    updatePhysicsTransform();
}

void Entity::moveTo(const vec3& position)
{
    invalidateChildrenCachedParentTransform();
    m_transform.position = position;
    updatePhysicsTransform();
}

void Entity::rotate(float angle, vec3 axis)
{
    invalidateChildrenCachedParentTransform();
    m_transform.rotate(angle, axis);
    updatePhysicsTransform();
}

void Entity::rotate(vec3 rotation)
{
    invalidateChildrenCachedParentTransform();
    m_transform.rotate(rotation);
    updatePhysicsTransform();
}

void Entity::rotateAround(vec3 pivot, vec3 rotation)
{
    invalidateChildrenCachedParentTransform();
    m_transform.rotateAround(pivot, rotation);
    updatePhysicsTransform();
}

void Entity::lookAt(const vec3& target)
{
    invalidateChildrenCachedParentTransform();
    m_transform.lookAt(target);
    updatePhysicsTransform();
}

void Entity::scale(vec3 s)
{
    invalidateChildrenCachedParentTransform();
    m_transform.scale(s);
    updatePhysicsTransform();
}

void Entity::scale(float s)
{
    invalidateChildrenCachedParentTransform();
    m_transform.scale(s);
    updatePhysicsTransform();
}

void Entity::setParent(const Entity* parent)
{
    invalidateCachedParentTransform();
    m_parent = parent;
}

void Entity::invalidateCachedParentTransform()
{
    m_cachedParentTransform.reset();

    invalidateChildrenCachedParentTransform();
}

void Entity::invalidateChildrenCachedParentTransform()
{
    for(const auto& child: m_children) {
        child->invalidateCachedParentTransform();
    }
}

void Entity::updatePhysicsTransform()
{
    if(auto rigidDynamic = dynamic_cast<physx::PxRigidDynamic*>(physicsActor.get())) {
        rigidDynamic->setGlobalPose(physics::toTransform(globalTransform()));
    }
}

bool EntityAnimation::update(double deltaTime, Entity& target)
{
    m_animationTime += deltaTime;
    return runAnimation(target);
}

ScaleAnimation::ScaleAnimation(double duration, vec3 startScale, vec3 endScale)
    : EntityAnimation()
    , m_duration(duration)
    , m_startScale(startScale)
    , m_endScale(endScale)
{
}

bool ScaleAnimation::runAnimation(Entity& target)
{
    auto t = m_animationTime / m_duration;
    auto factor = std::clamp(t, 0., 1.);

    auto transform = target.transform();
    transform.scaling = glm::mix(m_startScale, m_endScale, factor);
    target.setTransform(transform);

    return t <= 1.;
}

void AnimatableEntity::update(double deltaTime)
{
    if(m_animation) {
        auto ret = m_animation->update(deltaTime, *this);
        if(!ret) {
            m_animation.reset();
            if(m_animationFinisher) (*m_animationFinisher)();
            m_animationFinisher.reset();
        }
    }
}

} // namespace raygun
