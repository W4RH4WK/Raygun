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
#include "raygun/input/input_system.hpp"
#include "raygun/ui/text.hpp"

namespace raygun::ui {

class Window;
class Widget;
class Text;
class Button;
class CheckBox;
class Slider;

class Layout {
  public:
    Layout(vec2 position, vec2 offset, float scale = 1.f);
    void place(Entity& ent);

    void applyOffset(vec2 off);

  private:
    vec2 position;
    vec2 offset;
    float scale;
};

class Factory {
  public:
    Factory(std::shared_ptr<Font> font);

    std::shared_ptr<Window> window(string_view name, string_view title, float headerScale = 1.f) const;
    std::shared_ptr<Window> window(string_view name) const;
    std::shared_ptr<Text> text(string_view text, Alignment align = Alignment::TopLeft) const;
    std::shared_ptr<Button> button(string_view caption, const std::function<void()>& action, float minWidth = 0.f) const;
    std::shared_ptr<CheckBox> checkbox(string_view caption, float minWidth = 0.f) const;
    std::shared_ptr<Slider> slider(float width, double& value, double min = 0, double max = 1, double step = 0.1) const;

    std::shared_ptr<render::Model> getModel(const char* name) const;

    const TextGenerator& textGen() const;

    void addWithLayout(Entity& container, Layout l, std::function<void(Factory&)> orders);
    void applyOffset(vec2 offset);

  private:
    std::shared_ptr<Font> font;
    UniqueTextGenerator m_textGen;
    std::unordered_map<std::string, std::shared_ptr<render::Model>> models;

    mutable std::optional<Layout> currentLayout;
    mutable Entity* currentContainer = nullptr;
};

class Window : public AnimatableEntity {
    friend class Factory;

  public:
    void doLayout();

  private:
    string title;
    Window(const Factory& factory, string_view name, string_view title, float headerScale, bool includeDecorations = true);
    void forAllWidgets(const std::function<void(Widget&)>& f);
};

class Widget : public Entity {
  public:
    void setUIPosition(vec2 pos);
    vec2 UIPosition();

    virtual void doLayout(const Window&) {}

  protected:
    Widget(string_view name) : Entity(name) {}
};

class Text : public Widget {
    friend class Factory;

  private:
    string text;
    Text(const Factory& factory, string_view text, Alignment align);
};

class SelectableWidget : public Widget {
  public:
    virtual void select();
    virtual void deselect() { selected = false; }
    virtual bool isSelected() const { return selected; }

    virtual void doLayout(const Window& wnd);

    virtual bool runUI(double deltatime, input::Input input);

  private:
    SelectableWidget *upperWg = nullptr, *lowerWg = nullptr, *leftWg = nullptr, *rightWg = nullptr;
    bool selected = false;
    double timeSinceSelect = 0;

    std::shared_ptr<audio::Sound> selectSound;

  protected:
    SelectableWidget(string_view name);
};

class Button : public SelectableWidget {
    friend class Factory;

  public:
    void select() override;
    void deselect() override;
    bool runUI(double deltatime, input::Input input) override;

    void enableMultipress();
    void setCaption(std::string_view newCaption);

  private:
    const Factory& factory;
    string caption;
    std::shared_ptr<Entity> marker;
    std::function<void()> action;
    double timeSinceClick = 0;
    bool multiPress = false;

    std::shared_ptr<audio::Sound> clickSound;

    Button(const Factory& factory, string_view caption, const std::function<void()>& action, float minWidth);
};

class CheckBox : public SelectableWidget {
    friend class Factory;

  public:
    void select() override;
    void deselect() override;
    bool runUI(double deltatime, input::Input input) override;

  private:
    string caption;
    bool checked = false;
    double timeSinceCheck = 0;
    std::shared_ptr<Entity> marker;
    std::shared_ptr<Entity> checkmark;
    CheckBox(const Factory& factory, string_view caption, float minWidth);
};

class Slider : public SelectableWidget {
    friend class Factory;

  public:
    void select() override;
    void deselect() override;
    bool runUI(double deltatime, input::Input input) override;

    void setSmooth(bool smoothVal);

  private:
    bool active = false;
    bool smooth = false;
    double timeSinceAction = 0;
    double& value;
    double min, max, step;
    float sliderWidth;
    std::shared_ptr<Entity> marker;
    std::shared_ptr<Entity> sliderMarker;
    std::shared_ptr<Entity> sliderMarkerActive;

    Slider(const Factory& factory, float width, double& value, double min, double max, double step);
    void moveSliderMarkers();
};

/// Runs UI on all widgets in the scenegraph rooted in "root"
/// return "true" if some UI entity consumed the input,
/// in which case it shouldn't be used anymore (e.g. for movement)

bool runUI(Entity& root, double deltatime, input::Input input);

/// Returns a window that can be used for UI testing

std::shared_ptr<Window> uiTestWindow(Factory& factory);

} // namespace raygun::ui
