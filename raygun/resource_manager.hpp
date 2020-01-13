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

#include "raygun/audio/sound.hpp"
#include "raygun/entity.hpp"
#include "raygun/gpu/shader.hpp"
#include "raygun/material.hpp"
#include "raygun/render/model.hpp"
#include "raygun/ui/text.hpp"

namespace raygun {

/// A resource manager that caches resources on load.
class ResourceManager {
  public:
    /// Convenience function for loading entities.
    template<typename T = Entity>
    std::shared_ptr<T> loadEntity(string_view name)
    {
        return std::make_shared<T>(name, entityLoadPath(name));
    }

    /// All models not obtained via the resource manager must be registered,
    /// otherwise they will not be added to the GPU vertex buffer for rendering
    /// on scene load.
    void registerModel(std::shared_ptr<render::Model> model);

    /// Returns a list of all registered models.
    std::vector<render::Model*> models();

    void clearUnusedModelsAndMaterials();

    std::shared_ptr<Material> loadMaterial(string_view name);

    /// Returns a list of all loaded materials.
    std::vector<Material*> materials();

    std::shared_ptr<gpu::Shader> loadShader(string_view name);

    void clearShaderCache();

    std::shared_ptr<ui::Font> loadFont(string_view name);

    std::shared_ptr<audio::Sound> loadSound(string_view name);

    fs::path entityLoadPath(string_view name);

  private:
    std::set<std::shared_ptr<render::Model>> m_loadedModels;

    std::map<string, std::shared_ptr<Material>> m_materialCache;

    std::map<string, std::shared_ptr<gpu::Shader>> m_shaderCache;

    std::map<string, std::shared_ptr<ui::Font>> m_fontCache;

    std::map<string, std::shared_ptr<audio::Sound>> m_soundCache;
};

using UniqueResourceManager = std::unique_ptr<ResourceManager>;

} // namespace raygun
