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

#include "ui.hpp"

#include "raygun/raygun.hpp"
#include "raygun/resource_manager.hpp"
#include "raygun/utils/array_utils.hpp"

namespace raygun::ui {

namespace mesh_names {
    constexpr const char* WND = "window";
    constexpr const char* FOOTER = "footer";

    constexpr const char* HEADER_BG = "header_bg";
    constexpr const char* HEADER_TOP = "header_top";
    constexpr const char* HEADER_BOT = "header_bot";

    constexpr const char* BTN_CENTER = "button_center";
    constexpr const char* BTN_SIDE = "button_side";
    constexpr const char* BTN_MARKER = "button_marker";

    constexpr const char* CHK_LEFT = "checkbox_left";
    constexpr const char* CHECKMARK = "checkmark";

    constexpr const char* SLIDER_BAR = "slider_bar";
    constexpr const char* SLIDER_SIDE = "slider_side";
    constexpr const char* SLIDER_MARKER = "slider_marker";
    constexpr const char* SLIDER_MARKER_INACTIVE = "slider_marker_inactive";
} // namespace mesh_names

// Static reflection would be nice
constexpr std::array<const char*, 2 + 3 + 3 + 2 + 4> MESH_NAMES = {
    // clang-format off

    mesh_names::WND,
    mesh_names::FOOTER,

    mesh_names::HEADER_BG,
    mesh_names::HEADER_TOP,
    mesh_names::HEADER_BOT,

    mesh_names::BTN_CENTER,
    mesh_names::BTN_SIDE,
    mesh_names::BTN_MARKER,

    mesh_names::CHK_LEFT,
    mesh_names::CHECKMARK,

    mesh_names::SLIDER_BAR,
    mesh_names::SLIDER_SIDE,
    mesh_names::SLIDER_MARKER,
    mesh_names::SLIDER_MARKER_INACTIVE,

    // clang-format on
};

namespace {
    static constexpr vec3 WND_AREA_TOPLEFT = vec3(-1.f, 0.4f, 0.01f);
    static constexpr vec3 WND_AREA_BOTRIGHT = vec3(1.f, -0.6f, 0.01f);
    static constexpr float WND_WIDTH = WND_AREA_BOTRIGHT.x - WND_AREA_TOPLEFT.x;
    static constexpr float WND_HEIGHT = WND_AREA_TOPLEFT.y - WND_AREA_BOTRIGHT.y;

    constexpr vec3 toWndPos(vec2 coords)
    {
        vec3 ret = WND_AREA_TOPLEFT;
        ret.x += WND_WIDTH * coords.x;
        ret.y -= WND_HEIGHT * coords.y;
        return ret;
    }

    constexpr vec2 toWndCoords(vec3 pos)
    {
        return {
            (pos.x - WND_AREA_TOPLEFT.x) / WND_WIDTH,
            (pos.y - WND_AREA_TOPLEFT.y) / -WND_HEIGHT,
        };
    }

    static constexpr float WND_HDR_START_Y = 0.61f;
    static constexpr float WND_HDR_HEIGHT = 0.17f;

    static constexpr float BTN_PADDING = 0.2f;
    static constexpr float BTN_BASE_WIDTH = 0.3f;
    static constexpr float SLIDER_BAR_BASE_WIDTH = 0.24f;

    static constexpr float UI_TEXT_SCALE = 0.15f;

    static constexpr double UI_INTERACT_GRANULARITY = 0.2;

} // namespace

/////////////////////////////////////////////////////////////////////////////////////////////////// Layout

Layout::Layout(vec2 position, vec2 offset, float scale) : position(position), offset(offset), scale(scale) {}

void Layout::place(Entity& ent)
{
    ent.scale(scale);
    ent.moveTo(toWndPos(position));
    position += offset;
}

void Layout::applyOffset(vec2 off)
{
    position += off;
}

/////////////////////////////////////////////////////////////////////////////////////////////////// Factory

Factory::Factory(std::shared_ptr<Font> font) : font(font)
{
    auto uiEntity = RG().resourceManager().loadEntity("ui");
    for(const auto& mn: MESH_NAMES) {
        uiEntity->forEachEntity([&](Entity& e) {
            if(e.name == mn) {
                models[mn] = e.model;
                return;
            }
        });
        if(models.find(mn) == models.end()) {
            RAYGUN_ERROR("Could not find UI model {}.", mn);
        }
    }

    m_textGen = std::make_unique<TextGenerator>(*font, RG().resourceManager().loadMaterial("ui_text"));
}

std::shared_ptr<Window> Factory::window(string_view name, string_view title, float headerScale) const
{
    std::shared_ptr<Window> window(new Window(*this, name, title, headerScale));
    if(currentLayout) currentLayout->place(*window);
    if(currentContainer) currentContainer->addChild(window);
    return window;
}

std::shared_ptr<Window> Factory::window(string_view name) const
{
    std::shared_ptr<Window> window(new Window(*this, name, "", 0.f, false));
    if(currentLayout) currentLayout->place(*window);
    if(currentContainer) currentContainer->addChild(window);
    return window;
}

std::shared_ptr<Text> Factory::text(string_view text, Alignment align) const
{
    std::shared_ptr<Text> ret(new Text(*this, text, align));
    if(currentLayout) currentLayout->place(*ret);
    if(currentContainer) currentContainer->addChild(ret);
    return ret;
}

std::shared_ptr<Button> Factory::button(string_view caption, const std::function<void()>& action, float minWidth) const
{
    std::shared_ptr<Button> button(new Button(*this, caption, action, minWidth));
    if(currentLayout) currentLayout->place(*button);
    if(currentContainer) currentContainer->addChild(button);
    return button;
}

std::shared_ptr<raygun::ui::CheckBox> Factory::checkbox(string_view caption, float minWidth) const
{
    std::shared_ptr<CheckBox> checkbox(new CheckBox(*this, caption, minWidth));
    if(currentLayout) currentLayout->place(*checkbox);
    if(currentContainer) currentContainer->addChild(checkbox);
    return checkbox;
}

std::shared_ptr<raygun::ui::Slider> Factory::slider(float width, double& value, double min, double max, double step) const
{
    std::shared_ptr<Slider> slider(new Slider(*this, width, value, min, max, step));
    if(currentLayout) currentLayout->place(*slider);
    if(currentContainer) currentContainer->addChild(slider);
    return slider;
}

std::shared_ptr<raygun::render::Model> Factory::getModel(const char* name) const
{
    RAYGUN_ASSERT(models.find(name) != models.end());
    return models.find(name)->second;
}

const TextGenerator& Factory::textGen() const
{
    return *m_textGen;
}

void Factory::addWithLayout(Entity& container, Layout l, std::function<void(Factory&)> orders)
{
    currentContainer = &container;
    currentLayout = l;
    orders(*this);
    currentContainer = nullptr;
    currentLayout.reset();
}

void Factory::applyOffset(vec2 offset)
{
    if(currentLayout) {
        currentLayout->applyOffset(offset);
    }
    else {
        RAYGUN_WARN("Factory::applyOffset without active layout");
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////// Window

Window::Window(const Factory& factory, string_view name, string_view title, float headerScale, bool includeDecorations) : AnimatableEntity(name), title(title)
{
    std::shared_ptr<Entity> wnd = std::make_shared<Entity>(string(name) + "_wnd");
    wnd->model = factory.getModel(mesh_names::WND);
    addChild(wnd);

    // header + footer + title
    if(includeDecorations) {
        std::shared_ptr<Entity> header = std::make_shared<Entity>(string(name) + "_header_bg");
        header->model = factory.getModel(mesh_names::HEADER_BG);
        header->scale(vec3(1, headerScale, 1));
        header->moveTo(vec3(0, WND_HDR_START_Y, 0));
        addChild(header);

        std::shared_ptr<Entity> headerTop = std::make_shared<Entity>(string(name) + "_header_top");
        headerTop->model = factory.getModel(mesh_names::HEADER_TOP);
        addChild(headerTop);

        std::shared_ptr<Entity> headerBot = std::make_shared<Entity>(string(name) + "_header_bot");
        headerBot->model = factory.getModel(mesh_names::HEADER_BOT);
        headerBot->moveTo(vec3(0, WND_HDR_START_Y - WND_HDR_HEIGHT * headerScale, 0));
        addChild(headerBot);

        std::shared_ptr<Entity> footer = std::make_shared<Entity>(string(name) + "_footer");
        footer->model = factory.getModel(mesh_names::FOOTER);
        addChild(footer);

        auto titleEntity = factory.textGen().text(title, Alignment::BottomCenter);
        titleEntity->scale(vec3(UI_TEXT_SCALE * headerScale));
        titleEntity->moveTo(vec3(0, WND_HDR_START_Y - 0.03f * headerScale, 0.01f));
        addChild(titleEntity);
    }
}

void Window::doLayout()
{
    bool widgetSelected = false;
    SelectableWidget* firstSelectable = nullptr;

    forAllWidgets([&](Widget& w) {
        w.doLayout(*this);

        if(auto s = dynamic_cast<SelectableWidget*>(&w)) {
            if(!firstSelectable) firstSelectable = s;
            if(s->isSelected()) {
                if(widgetSelected) RAYGUN_WARN("More than one selectable widget selected in Window!");
                widgetSelected = true;
            }
        }
        if(!widgetSelected && firstSelectable) firstSelectable->select();
    });
}

void Window::forAllWidgets(const std::function<void(Widget&)>& f)
{
    forEachEntity([&](Entity& e) {
        if(auto widget = dynamic_cast<Widget*>(&e)) {
            f(*widget);
        }

        // descend only one level
        return &e == this;
    });
}

/////////////////////////////////////////////////////////////////////////////////////////////////// Widget

void Widget::setUIPosition(vec2 pos)
{
    moveTo(toWndPos(pos));
}

raygun::vec2 Widget::UIPosition()
{
    return toWndCoords(transform().position);
}

/////////////////////////////////////////////////////////////////////////////////////////////////// Text

Text::Text(const Factory& factory, string_view text, Alignment align) : Widget("ui_text_" + string(text)), text(text)
{
    auto textEntity = factory.textGen().text(text, align);
    textEntity->scale(UI_TEXT_SCALE);
    addChild(textEntity);
}

/////////////////////////////////////////////////////////////////////////////////////////////////// SelectableWidget

void SelectableWidget::doLayout(const Window& wnd)
{
    auto myCoords = UIPosition();

    // find the "closest" upper, lower, left and right selectable widget from this one
    // (if one is available)
    // this is not particularly runtime-optimized, but the expectation is to at most
    // have a few dozen active widgets per container, so it doesn't matter

    SelectableWidget** targets[] = {&upperWg, &lowerWg, &leftWg, &rightWg};
    std::function<bool(vec2)> filters[] = {
        [](vec2 dir) { return glm::normalize(dir).y <= -0.5f; },
        [](vec2 dir) { return glm::normalize(dir).y >= 0.5f; },
        [](vec2 dir) { return glm::normalize(dir).x <= -0.5f; },
        [](vec2 dir) { return glm::normalize(dir).x >= 0.5f; },
    };
    static_assert(RAYGUN_ARRAY_COUNT(filters) == RAYGUN_ARRAY_COUNT(targets));
    vec2 weights[] = {{1.f, 0.25f}, {1.f, 0.25f}, {0.25f, 1.f}, {0.25f, 1.f}};
    static_assert(RAYGUN_ARRAY_COUNT(weights) == RAYGUN_ARRAY_COUNT(targets));

    // iterate over up, down, left, right, same search procedure for each one
    for(size_t i = 0; i < RAYGUN_ARRAY_COUNT(targets); ++i) {
        SelectableWidget* best = nullptr;
        float bestDistance = std::numeric_limits<float>::max();

        for(const auto& child: wnd.children()) {
            if(auto swg = dynamic_cast<SelectableWidget*>(&*child)) {
                if(swg == this) continue;
                vec2 dir = swg->UIPosition() - myCoords;
                if(!filters[i](dir)) continue;
                float scaledDist = glm::dot(glm::abs(dir), weights[i]);
                if(scaledDist < bestDistance) {
                    bestDistance = scaledDist;
                    best = swg;
                }
            }
        }
        *targets[i] = best;
    }
}

void SelectableWidget::select()
{
    if(!selected) {
        RG().audioSystem().playSoundEffect(selectSound);
    }

    selected = true;
    timeSinceSelect = 0;
}

bool SelectableWidget::runUI(double deltatime, input::Input input)
{
    timeSinceSelect += deltatime;

    if(selected && timeSinceSelect > UI_INTERACT_GRANULARITY) {

        SelectableWidget* targets[] = {upperWg, lowerWg, leftWg, rightWg};
        bool inputs[] = {input.up(), input.down(), input.left(), input.right()};
        static_assert(RAYGUN_ARRAY_COUNT(targets) == RAYGUN_ARRAY_COUNT(inputs));

        // iterate over up, down, left, right, same search procedure for each one
        for(size_t i = 0; i < RAYGUN_ARRAY_COUNT(targets); ++i) {
            if(targets[i] && inputs[i]) {
                deselect();
                targets[i]->select();
                return true;
            }
        }
    }
    return false;
}

SelectableWidget::SelectableWidget(string_view name) : Widget(name), selectSound(RG().resourceManager().loadSound("ui_button_select")) {}

/////////////////////////////////////////////////////////////////////////////////////////////////// Button

namespace {

    void buildHorizontalElement(Entity& base, const Factory& factory, float halfWidth, float baseWidth, const char* leftModel, const char* centerModel,
                                const char* rightModel)
    {
        std::shared_ptr<Entity> center = std::make_shared<Entity>(base.name + "_center");
        center->model = factory.getModel(centerModel);
        center->scale(vec3(halfWidth / baseWidth, 1, 1));
        base.addChild(center);

        std::shared_ptr<Entity> left = std::make_shared<Entity>(base.name + "_left");
        left->model = factory.getModel(leftModel);
        left->moveTo(vec3(baseWidth - halfWidth, 0, 0));
        base.addChild(left);

        std::shared_ptr<Entity> right = std::make_shared<Entity>(base.name + "_right");
        right->model = factory.getModel(rightModel);
        right->rotate(glm::radians(180.f), vec3(0, 0, 1));
        right->moveTo(vec3(-baseWidth + halfWidth, 0, 0));
        base.addChild(right);
    }

    float buildWidgetWithCaption(Entity& base, const Factory& factory, string_view caption, float minWidth, std::shared_ptr<Entity>& marker, bool isCheckbox)
    {
        float halfWidth = minWidth / 2;

        if(!caption.empty()) {
            auto [captionEntity, captionBounds] = factory.textGen().textWithBounds(caption, Alignment::MiddleCenter);
            captionEntity->move(vec3(0, 0, 0.01f));
            captionEntity->scale(UI_TEXT_SCALE);
            base.addChild(captionEntity);

            halfWidth = std::max((captionBounds.upper.x + BTN_PADDING) * UI_TEXT_SCALE, minWidth / 2);
        }

        auto leftModel = isCheckbox ? mesh_names::CHK_LEFT : mesh_names::BTN_SIDE;
        buildHorizontalElement(base, factory, halfWidth, BTN_BASE_WIDTH, leftModel, mesh_names::BTN_CENTER, mesh_names::BTN_SIDE);

        {
            marker = base.emplaceChild(base.name + "_marker");
            marker->hide();

            if(!isCheckbox) {
                std::shared_ptr<Entity> markLeft = std::make_shared<Entity>(base.name + "_marker_left");
                markLeft->model = factory.getModel(mesh_names::BTN_MARKER);
                markLeft->moveTo(vec3(BTN_BASE_WIDTH - halfWidth, 0, 0));
                marker->addChild(markLeft);
            }

            std::shared_ptr<Entity> markRight = std::make_shared<Entity>(base.name + "_marker_right");
            markRight->model = factory.getModel(mesh_names::BTN_MARKER);
            markRight->rotate(glm::radians(180.f), vec3(0, 0, 1));
            markRight->moveTo(vec3(-BTN_BASE_WIDTH + halfWidth, 0, 0));
            marker->addChild(markRight);
        }

        return halfWidth;
    }
} // namespace

Button::Button(const Factory& factory, string_view caption, const std::function<void()>& action, float minWidth)
    : SelectableWidget("btn_" + string(caption))
    , factory(factory)
    , action(action)
    , clickSound(RG().resourceManager().loadSound("ui_button_click"))
{
    buildWidgetWithCaption(*this, factory, caption, minWidth, marker, false);
}

void Button::select()
{
    SelectableWidget::select();
    marker->show();
}

void Button::deselect()
{
    SelectableWidget::deselect();
    marker->hide();
}

void Button::enableMultipress()
{
    multiPress = true;
}

void Button::setCaption(std::string_view newCaption)
{
    caption = newCaption;

    auto newCaptionEntity = factory.textGen().text(caption, Alignment::MiddleCenter);
    newCaptionEntity->move(vec3(0, 0, 0.01f));
    newCaptionEntity->scale(UI_TEXT_SCALE);

    replaceChild(children().front(), newCaptionEntity);
}

bool Button::runUI(double deltatime, input::Input input)
{
    bool consumed = false;
    consumed |= SelectableWidget::runUI(deltatime, input);

    timeSinceClick += deltatime;
    if(isSelected()) {
        if(input.ok && timeSinceClick > UI_INTERACT_GRANULARITY) {
            RG().audioSystem().playSoundEffect(clickSound);
            action();
            timeSinceClick = 0;
            consumed = true;
            if(!multiPress) deselect();
        }
    }

    return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////// CheckBox

CheckBox::CheckBox(const Factory& factory, string_view caption, float minWidth) : SelectableWidget("chk_" + string(caption))
{
    float halfWidth = buildWidgetWithCaption(*this, factory, caption, minWidth, marker, true);

    checkmark = std::make_shared<Entity>(name + "_checkmark");
    checkmark->model = factory.getModel(mesh_names::CHECKMARK);
    checkmark->moveTo(vec3(BTN_BASE_WIDTH - halfWidth, 0, 0));
    checkmark->setVisible(checked);
    addChild(checkmark);
}

void CheckBox::select()
{
    SelectableWidget::select();
    marker->show();
}

void CheckBox::deselect()
{
    SelectableWidget::deselect();
    marker->hide();
}

bool CheckBox::runUI(double deltatime, input::Input input)
{
    bool consumed = false;
    consumed |= SelectableWidget::runUI(deltatime, input);

    timeSinceCheck += deltatime;
    if(isSelected() && timeSinceCheck > UI_INTERACT_GRANULARITY) {
        if(input.ok) {
            timeSinceCheck = 0;
            consumed = true;
            checked = !checked;
            checkmark->setVisible(checked);
        }
    }
    return consumed;
}

/////////////////////////////////////////////////////////////////////////////////////////////////// Slider

Slider::Slider(const Factory& factory, float width, double& value, double min, double max, double step)
    : SelectableWidget("slider")
    , value(value)
    , min(min)
    , max(max)
    , step(step)
{
    buildWidgetWithCaption(*this, factory, "", width, marker, false);

    sliderWidth = width / 2 - (BTN_BASE_WIDTH - SLIDER_BAR_BASE_WIDTH);
    buildHorizontalElement(*this, factory, width / 2 - (BTN_BASE_WIDTH - SLIDER_BAR_BASE_WIDTH), SLIDER_BAR_BASE_WIDTH, mesh_names::SLIDER_SIDE,
                           mesh_names::SLIDER_BAR, mesh_names::SLIDER_SIDE);

    sliderMarker = emplaceChild("slider_marker");
    sliderMarker->model = factory.getModel(mesh_names::SLIDER_MARKER_INACTIVE);

    sliderMarkerActive = emplaceChild("slider_marker_active");
    sliderMarkerActive->model = factory.getModel(mesh_names::SLIDER_MARKER);
    sliderMarkerActive->hide();

    moveSliderMarkers();
}

void Slider::select()
{
    SelectableWidget::select();
    marker->show();
}

void Slider::deselect()
{
    SelectableWidget::deselect();
    marker->hide();
}

bool Slider::runUI(double deltatime, input::Input input)
{
    bool consumed = false;
    if(!active) {
        consumed |= SelectableWidget::runUI(deltatime, input);
    }

    timeSinceAction += deltatime;
    if(isSelected() && timeSinceAction > UI_INTERACT_GRANULARITY) {
        if(input.ok) {
            active = !active;
            sliderMarkerActive->setVisible(active);
            marker->setVisible(!active);
            timeSinceAction = 0;
            consumed = true;
        }
    }

    if(active && (smooth || timeSinceAction > UI_INTERACT_GRANULARITY)) {
        if(input.right() || input.left()) {
            double off = input.left() ? -step : step;
            if(smooth) off *= fabs(input.dir.x) * deltatime;
            value += off;
            timeSinceAction = 0;
            consumed = true;
        }
    }

    value = std::clamp(value, min, max);
    moveSliderMarkers();

    return consumed;
}

void Slider::moveSliderMarkers()
{
    auto pos = -sliderWidth + sliderWidth * 2 * ((value - min) / (max - min));
    sliderMarker->moveTo(vec3(pos, 0, 0));
    sliderMarkerActive->moveTo(vec3(pos, 0, 0));
}

void Slider::setSmooth(bool smoothVal)
{
    smooth = smoothVal;
}

/////////////////////////////////////////////////////////////////////////////////////////////////// General

bool runUI(Entity& root, double deltatime, input::Input input)
{
    bool consumed = false;
    root.forEachEntity([&](Entity& ent) {
        if(auto s = dynamic_cast<SelectableWidget*>(&ent)) {
            consumed |= s->runUI(deltatime, input);
        }
    });
    return consumed;
}

namespace {
    double sval = 3;
}

std::shared_ptr<Window> uiTestWindow(Factory& factory)
{
    auto wnd = factory.window("test_window", "Testing Window");

    factory.addWithLayout(*wnd, ui::Layout(vec2(0.5f, 0.1f), vec2(0.f, 0.15f), 0.6f), [](ui::Factory& f) {
        f.button("First", [] {});
        f.button("Second", [] {});
        f.button("Third", [] {});
        f.slider(0.9f, sval, 1, 9, 1);
    });

    factory.addWithLayout(*wnd, ui::Layout(vec2(0.05f, 0.72f), vec2(0.3f, 0.f), 0.9f), [](ui::Factory& f) {
        f.text("Horizontal:");
        f.applyOffset(vec2(-0.15f, 0.13f));
        f.button(
            "X", [] {}, 0.2f);
        f.button(
            "Y", [] {}, 0.2f);
        f.button(
            "Z", [] {}, 0.2f);
    });

    factory.addWithLayout(*wnd, ui::Layout(vec2(0.2f, 0.1f), vec2(0.f, 0.2f), 0.7f), [](ui::Factory& f) {
        f.checkbox("Foo", 0.45f);
        f.checkbox("FXAA", 0.45f);
    });

    wnd->doLayout();
    wnd->setAnimation(ScaleAnimation(0.25, vec3(1, 0, 1), vec3(1)));
    wnd->moveTo(vec3(0, 0, -2.5));

    return wnd;
}

} // namespace raygun::ui
