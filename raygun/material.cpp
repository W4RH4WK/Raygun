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

#include "raygun/material.hpp"

#include "raygun/logging.hpp"
#include "raygun/physics/physics_utils.hpp"
#include "raygun/raygun.hpp"
#include "raygun/utils/json_utils.hpp"

namespace raygun {

namespace {
    template<typename T>
    void loadMaterialParam(const json& value, T& out)
    {
        if constexpr(std::is_same_v<std::remove_reference_t<T>, vec3>) {
            out = toVec3(value);
        }
        else {
            out = value;
        }
    }

    void copyParameters(Material& dst, const Material& src)
    {
        dst.gpuMaterial = src.gpuMaterial;

        dst.physicsMaterial = physics::cloneMaterial(RG().physicsSystem().physics(), *src.physicsMaterial);
        dst.physicsMaterial->userData = (void*)&dst;
    }

} // namespace

Material::Material() : physicsMaterial(RG().physicsSystem().physics().createMaterial(0.8f, 0.8f, 0.6f))
{
    physicsMaterial->userData = static_cast<void*>(this);
}

Material::Material(string_view name, const fs::path& path) : Material()
{
    this->name = name;

    std::ifstream in(path);
    if(!in) {
        RAYGUN_ERROR("Unable to open: {}", path);
        return;
    }

    json data;
    in >> data;

    if(data.at("type") != "Material") {
        RAYGUN_ERROR("Not a material: {}", path);
        return;
    }

    if(data.contains("basedOn")) {
        const auto baseMaterial = RG().resourceManager().loadMaterial(data.at("basedOn"));
        copyParameters(*this, *baseMaterial);
    }

    for(const auto& [key, value]: data.items()) {
        if(key == "type") continue;
        if(key == "basedOn") continue;

            // gpu::Material parameters:
#define MAT_PARAM(_type, _name, _default, _min, _max) \
    if(key == #_name) { \
        loadMaterialParam(value, gpuMaterial._name); \
        continue; \
    }
#define MAT_PARAM_PAD(_type, _name)
#include "resources/shaders/gpu_material.def"

        // physics material parameters:
        if(key == "staticFriction") {
            physicsMaterial->setStaticFriction(value);
            continue;
        }
        else if(key == "dynamicFriction") {
            physicsMaterial->setDynamicFriction(value);
            continue;
        }

        RAYGUN_WARN("Unknown field '{}' in material: {}", key, path);
    }
}

std::vector<physx::PxMaterial*> collectPhysicsMaterials(const std::vector<std::shared_ptr<Material>>& materials)
{
    const auto toPtr = [](const std::shared_ptr<Material>& material) { return material->physicsMaterial.get(); };

    std::vector<physx::PxMaterial*> result(materials.size());
    std::transform(materials.begin(), materials.end(), result.begin(), toPtr);
    return result;
}

} // namespace raygun
