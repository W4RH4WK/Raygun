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

#include "raygun/animation/animator.hpp"
#include "raygun/audio/audio_source.hpp"
#include "raygun/physics/physics_utils.hpp"
#include "raygun/render/model.hpp"
#include "raygun/transform.hpp"

namespace raygun {

class Entity {
  public:
    explicit Entity(string_view name);

    /// Loads the given entity by path, all containing models are automatically
    /// registered with the ResourceManager. Materials are loaded via their name
    /// automatically.
    Entity(string_view name, fs::path filepath, bool loadMaterials = true);

    virtual ~Entity() {}

    const Transform& transform() const { return m_transform; }
    void setTransform(Transform transform);

    /// Returns the accumulated Transform of all (direct and transitive) parents.
    Transform parentTransform() const;

    /// Returns the accumulated Transform of all (direct and transitive) parents and self.
    Transform globalTransform() const;

    bool isVisible() const { return m_visible; }
    void setVisible(bool visible) { m_visible = visible; }
    void show() { setVisible(true); }
    void hide() { setVisible(false); }

    const std::vector<std::shared_ptr<Entity>>& children() const { return m_children; }

    void addChild(std::shared_ptr<Entity> child);
    std::shared_ptr<Entity> emplaceChild(string_view childName = {});

    void replaceChild(const std::shared_ptr<Entity>& oldChild, std::shared_ptr<Entity> newChild);

    void removeChild(const std::shared_ptr<Entity>& child);
    void clearChildren();

    template<typename Fun>
    void forEachEntity(Fun f)
    {
        bool descend = true;

        if constexpr(std::is_invocable_r_v<bool, Fun, Entity&>) {
            descend = f(*this);
        }
        else {
            f(*this);
        }

        if(!descend) return;

        for(auto& child: m_children) {
            child->forEachEntity(f);
        }
    }

    //////////////////////////////////////////////////////////////////////////

    void move(const vec3& translation);
    void moveTo(const vec3& position);

    void rotate(float angle, vec3 axis);
    void rotate(vec3 rotation);
    void rotateAround(vec3 pivot, vec3 rotation);
    void lookAt(const vec3& target);

    void scale(vec3 s);
    void scale(float s);

    //////////////////////////////////////////////////////////////////////////

    string name;

    std::shared_ptr<render::Model> model;

    physics::UniqueActor physicsActor;

    audio::UniqueSource audioSource;

    animation::UniqueTransformAnimator animator;

  private:
    void setParent(const Entity* parent);
    void clearParent() { setParent(nullptr); }

    void invalidateCachedParentTransform();
    void invalidateChildrenCachedParentTransform();

    void updatePhysicsTransform();

    Transform m_transform;

    bool m_visible = true;

    // Invariant: Pointer to parent needs to be set / cleared when adding /
    // removing children.
    const Entity* m_parent = nullptr;

    // Invariant: Cached parent transform needs to be cleared when parent
    // changes.
    mutable std::optional<Transform> m_cachedParentTransform;

    std::vector<std::shared_ptr<Entity>> m_children;
};

} // namespace raygun
