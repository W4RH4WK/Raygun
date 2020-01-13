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

#include "raygun/gpu/gpu_material.hpp"

#include "raygun/logging.hpp"
#include "raygun/material.hpp"
#include "raygun/raygun.hpp"
#include "raygun/utils/json_utils.hpp"

namespace raygun::gpu {

namespace {
    template<typename T>
    bool editMaterialParam(const char* name, T min, T max, T& inout)
    {
        if constexpr(std::is_same_v<std::remove_reference_t<T>, vec3>) {
            return ImGui::SliderFloat3(name, reinterpret_cast<float*>(&inout), min.x, max.x);
        }
        else if constexpr(std::is_same_v<std::remove_reference_t<T>, float>) {
            return ImGui::SliderFloat(name, &inout, min, max);
        }
        else if constexpr(std::is_same_v<std::remove_reference_t<T>, uint>) {
            return ImGui::SliderInt(name, (int*)&inout, (int)min, (int)max);
        }
        else {
            RAYGUN_WARN("Unexpected material type");
            return false;
        }
    }
} // namespace

void materialEditor()
{
    auto materials = RG().resourceManager().materials();
    if(materials.empty()) return;

    static const char* selection = nullptr;
    if(!selection) {
        selection = materials[0]->name.c_str();
    }

    auto changed = false;

    ImGui::Begin("Material Editor");

    if(ImGui::BeginCombo("Material", selection)) {
        for(const auto& material: materials) {
            auto selected = material->name == selection;

            if(ImGui::Selectable(material->name.c_str(), selected)) {
                selection = material->name.c_str();
            }

            if(selected) {
                ImGui::SetItemDefaultFocus();
            }
        }

        ImGui::EndCombo();
    }

    auto it = std::find_if(materials.begin(), materials.end(), [&](const auto& material) { return material->name == selection; });
    if(it != materials.end()) {
#define MAT_PARAM(_type, _name, _default, _min, _max) changed |= editMaterialParam<_type>(#_name, _min, _max, (*it)->gpuMaterial._name);
#define MAT_PARAM_PAD(_type, _name)
#include "resources/shaders/gpu_material.def"
    }

    ImGui::End();

    if(changed) {
        RG().renderSystem().updateModelBuffers();
    }
}

} // namespace raygun::gpu
