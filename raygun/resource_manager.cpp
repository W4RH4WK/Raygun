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

#include "raygun/resource_manager.hpp"

#include "raygun/logging.hpp"
#include "raygun/utils/assimp_utils.hpp"

namespace raygun {

const fs::path RESOURCES_DIR = "resources";

namespace {
    template<typename T>
    std::shared_ptr<T> loadFromFileSystemCached(string_view resourceType, const string& name, const fs::path& path, std::map<string, std::shared_ptr<T>>& cache)
    {
        const auto it = cache.find(name);
        if(it != cache.cend()) return it->second;

        RAYGUN_INFO("Loading {}: {}", resourceType, name);

        // search in alternative path if not found directly
        // allow e.g. materials/ui_button.rgmat.json ==> materials/ui/button.rgmat.json
        fs::path altPath = path;
        if(!fs::exists(RESOURCES_DIR / path)) {
            string ps = path.string();
            auto underscorePos = ps.find_first_of("_");
            if(underscorePos != ps.npos) {
                altPath = fs::path(ps.substr(0, underscorePos)) / ps.substr(underscorePos + 1);
            }
        }

        auto result = std::make_shared<T>(name, RESOURCES_DIR / altPath);

        cache[name] = result;

        return result;
    }
} // namespace

std::shared_ptr<Material> ResourceManager::loadMaterial(string_view nameView)
{
    const auto name = string{nameView};
    return loadFromFileSystemCached("Material", name, fs::path{"materials"} / (name + ".rgmat.json"), m_materialCache);
}

void ResourceManager::registerModel(std::shared_ptr<render::Model> model)
{
    m_loadedModels.insert(model);
}

std::vector<render::Model*> ResourceManager::models()
{
    constexpr auto raw = [](auto sptr) { return sptr.get(); };

    std::vector<render::Model*> result(m_loadedModels.size());
    std::transform(m_loadedModels.begin(), m_loadedModels.end(), result.begin(), raw);
    return result;
}

void ResourceManager::clearUnusedModelsAndMaterials()
{
    std::experimental::erase_if(m_loadedModels, [](const auto& sptr) { return sptr.use_count() <= 1; });

    std::experimental::erase_if(m_materialCache, [](const auto& pair) { return pair.second.use_count() <= 1; });
}

std::vector<Material*> ResourceManager::materials()
{
    constexpr auto snd = [](auto pair) { return &*pair.second; };

    std::vector<Material*> result(m_materialCache.size());
    std::transform(m_materialCache.begin(), m_materialCache.end(), result.begin(), snd);
    return result;
}

std::shared_ptr<gpu::Shader> ResourceManager::loadShader(string_view nameView)
{
    const auto name = string{nameView};
    return loadFromFileSystemCached("Shader", name, fs::path{"shaders"} / (name + ".spv"), m_shaderCache);
}

void ResourceManager::clearShaderCache()
{
    m_shaderCache.clear();
}

std::shared_ptr<ui::Font> ResourceManager::loadFont(string_view nameView)
{
    const auto name = string{nameView};

    const auto it = m_fontCache.find(name);
    if(it != m_fontCache.cend()) return it->second;

    auto result = std::make_shared<ui::Font>();
    result->name = name;

    auto entity = std::make_shared<Entity>(name, RESOURCES_DIR / "fonts" / (name + ".obj"), false);

    for(const auto& glyph: entity->children()) {
        const auto index = std::stoul(glyph->name);
        if(index >= result->charMap.size()) continue;

        const auto& mesh = glyph->model->mesh;
        auto bounds = mesh->bounds();
        for(auto& v: mesh->vertices) {
            v.position.x -= bounds.lower.x;
        }
        result->charMap[index] = mesh;
        result->charWidth[index] = mesh->width();
    }

    m_fontCache[name] = result;

    return result;
}

std::shared_ptr<audio::Sound> ResourceManager::loadSound(string_view nameView)
{
    const auto name = string{nameView};
    return loadFromFileSystemCached("Sound", name, fs::path{"sounds"} / (name + ".opus"), m_soundCache);
}

fs::path ResourceManager::entityLoadPath(string_view name)
{
    return RESOURCES_DIR / "models" / (string{name} + ".dae");
}

} // namespace raygun
