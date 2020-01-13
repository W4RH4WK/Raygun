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

#include "raygun/ui/text.hpp"

#include "raygun/assert.hpp"
#include "raygun/raygun.hpp"
#include "raygun/render/model.hpp"

namespace raygun::ui {

TextGenerator::TextGenerator(const Font& font, std::shared_ptr<Material> material, float letterPadding, float lineSpacing)
    : m_charWidth(font.charWidth)
    , letterPadding(letterPadding)
    , lineSpacing(lineSpacing)
{
    RAYGUN_ASSERT(font.charMap.size() == m_charMap.size());

    // Create a model from a mesh, adding the provided material.
    const auto meshToModel = [&](const auto& mesh) -> std::shared_ptr<render::Model> {
        if(!mesh) return {};

        auto model = std::make_shared<render::Model>();
        model->mesh = mesh;
        model->materials.push_back(material);

        RG().resourceManager().registerModel(model);

        return model;
    };

    std::transform(font.charMap.begin(), font.charMap.end(), m_charMap.begin(), meshToModel);
}

std::shared_ptr<raygun::Entity> TextGenerator::text(string_view input, Alignment align) const
{
    return textWithBounds(input, align).first;
}

std::pair<std::shared_ptr<Entity>, render::Mesh::Bounds> TextGenerator::textWithBounds(string_view input, Alignment align) const
{
    auto [textEnt, bounds] = textInternal(input);

    vec3 offset = vec3(0);
    switch(align) {
    case Alignment::TopLeft:
        offset = vec3(0, 0, 0);
        break;
    case Alignment::TopCenter:
        offset = vec3(-bounds.upper.x / 2, 0, 0);
        break;
    case Alignment::TopRight:
        offset = vec3(-bounds.upper.x, 0, 0);
        break;
    case Alignment::MiddleLeft:
        offset = vec3(0, -bounds.upper.y / 2, 0);
        break;
    case Alignment::MiddleCenter:
        offset = vec3(-bounds.upper.x / 2, -bounds.upper.y / 2, 0);
        break;
    case Alignment::MiddleRight:
        offset = vec3(-bounds.upper.x, -bounds.upper.y / 2, 0);
        break;
    case Alignment::BottomLeft:
        offset = vec3(0, -bounds.upper.y, 0);
        break;
    case Alignment::BottomCenter:
        offset = vec3(-bounds.upper.x / 2, -bounds.upper.y, 0);
        break;
    case Alignment::BottomRight:
        offset = vec3(-bounds.upper.x, -bounds.upper.y, 0);
        break;
    }

    textEnt->moveTo(offset);
    bounds.upper += offset;
    bounds.lower += offset;

    auto result = std::make_shared<Entity>("string_" + string(input));
    result->addChild(textEnt);
    return {result, bounds};
}

std::shared_ptr<render::Model> TextGenerator::letter(char c) const
{
    return m_charMap.at(c);
}

std::pair<std::shared_ptr<Entity>, render::Mesh::Bounds> TextGenerator::textInternal(string_view input) const
{
    auto result = std::make_shared<Entity>("char_group_" + string(input));
    render::Mesh::Bounds bounds;

    vec2 offset = {0.0f, 0.0f};
    for(const auto& c: input) {
        if(c == ' ') {
            offset.x += 5 * letterPadding;
        }
        else if(c == '\n') {
            offset.x = 0;
            offset.y -= lineSpacing;
            continue;
        }

        const auto model = letter(c);
        if(!model) continue;

        auto entity = result->emplaceChild(std::to_string(c));
        entity->move({offset, 0.0f});
        entity->model = model;

        offset.x += letterPadding + m_charWidth[c];

        bounds.upper.x = offset.x - letterPadding;
        bounds.upper.y = offset.y + lineSpacing * 0.66f;
    }

    return {result, bounds};
}

} // namespace raygun::ui
