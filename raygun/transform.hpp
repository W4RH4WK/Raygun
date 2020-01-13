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

namespace raygun {

struct Transform {
    Transform() {}
    explicit Transform(const vec3& position) : position(position) {}

    explicit Transform(const mat4& mat)
    {
        glm::vec3 skew;
        glm::vec4 perspective;
        glm::decompose(mat, scaling, rotation, position, skew, perspective);
    }

    mat4 toMat4() const
    {
        constexpr auto identity = glm::identity<mat4>();

        const auto p = glm::translate(identity, position);
        const auto r = glm::toMat4(rotation);
        const auto s = glm::scale(identity, scaling);
        return p * r * s;
    }

    vec3 eulerAngles() const { return glm::eulerAngles(rotation); }

    vec3 up() const { return glm::rotate(rotation, UP); }
    vec3 right() const { return glm::rotate(rotation, RIGHT); }
    vec3 forward() const { return glm::rotate(rotation, FORWARD); }

    Transform inverse() const
    {
        Transform result;
        result.position = -position;
        result.rotation = glm::inverse(rotation);
        result.scaling = 1.0f / scaling;
        return result;
    }

    bool isIdentity() const { return position == glm::zero<vec3>() && rotation == glm::identity<quat>() && scaling == glm::one<vec3>(); }

    bool isZeroVolume() const { return scaling.x * scaling.y * scaling.z == 0.0f; }

    //////////////////////////////////////////////////////////////////////////

    void move(const vec3& translation) { position += translation; }

    void rotate(float angle, vec3 axis) { rotation = glm::rotate(rotation, angle, axis); }

    void rotate(vec3 angles) { rotation = quat{angles} * rotation; }

    void rotateAround(vec3 pivot, vec3 angles)
    {
        move(-pivot);
        rotate(angles);
        move(pivot);
    }

    void lookAt(const vec3& target)
    {
        const auto direction = glm::normalize(target - position);
        rotation = glm::quatLookAt(direction, UP);
    }

    void scale(float factor) { scaling *= factor; }

    void scale(vec3 factors) { scaling *= factors; }

    //////////////////////////////////////////////////////////////////////////

    vec3 position = glm::zero<vec3>();
    quat rotation = glm::identity<quat>();
    vec3 scaling = glm::one<vec3>();
};

static inline Transform operator*(const Transform& x, const Transform& y)
{
    Transform result;
    result.position = glm::rotate(x.rotation, x.scaling * y.position) + x.position;
    result.rotation = x.rotation * y.rotation;
    result.scaling = x.scaling * y.scaling;
    return result;
}

static inline Transform interpolate(const Transform& x, const Transform& y, float factor)
{
    factor = std::clamp(factor, 0.f, 1.f);

    Transform result;
    result.position = glm::lerp(x.position, y.position, factor);
    result.rotation = glm::lerp(x.rotation, y.rotation, factor);
    result.scaling = glm::lerp(x.scaling, y.scaling, factor);
    return result;
}

} // namespace raygun
