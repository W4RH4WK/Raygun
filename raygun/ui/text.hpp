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

#include "raygun/entity.hpp"
#include "raygun/material.hpp"
#include "raygun/render/model.hpp"

namespace raygun::ui {

struct Font {
    string name;

    std::array<std::shared_ptr<render::Mesh>, 128> charMap = {};

    std::array<float, 128> charWidth = {};
};

enum class Alignment {
    TopLeft,
    TopCenter,
    TopRight,
    MiddleLeft,
    MiddleCenter,
    MiddleRight,
    BottomLeft,
    BottomCenter,
    BottomRight,
};

class TextGenerator {
  public:
    TextGenerator(const Font& font, std::shared_ptr<Material> material, float letterPadding = 0.1f, float lineSpacing = 1.f);

    std::shared_ptr<Entity> text(string_view input, Alignment align = Alignment::TopLeft) const;
    std::pair<std::shared_ptr<Entity>, render::Mesh::Bounds> textWithBounds(string_view input, Alignment align = Alignment::TopLeft) const;

  private:
    std::array<std::shared_ptr<render::Model>, 128> m_charMap = {};
    std::array<float, 128> m_charWidth = {};

    float letterPadding, lineSpacing;

    std::shared_ptr<render::Model> letter(char c) const;
    std::pair<std::shared_ptr<Entity>, render::Mesh::Bounds> textInternal(string_view input) const;
};

using UniqueTextGenerator = std::unique_ptr<TextGenerator>;

} // namespace raygun::ui
