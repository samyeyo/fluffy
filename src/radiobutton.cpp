// src/radiobutton.cpp

// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

// RadioButton — toggleable circular indicator with optional text label and group mutual exclusion.

#include <fluffy/radiobutton.hpp>
#include <fluffy/window.hpp>
#include <fluffy/yaml_fwd.hpp>
#include <nanovg.h>
#include <algorithm>
#include <unordered_map>
#include <vector>
namespace fluffy {

std::unordered_map<std::string, std::vector<RadioButton*>> RadioButton::s_groups;

// ––––––––[ RadioButton ] constructs with bounds and optional label string.
RadioButton::RadioButton(Rect bounds, std::string label)
    : Widget(bounds),
      label_(std::move(label))
{
    registerGroup();
}

// ––––––––[ registerGroup ] registers this button in the static group map.
void RadioButton::registerGroup() {
    if (group_.empty()) return;
    auto& vec = s_groups[group_];
    if (std::find(vec.begin(), vec.end(), this) == vec.end())
        vec.push_back(this);
}

// ––––––––[ unregisterGroup ] removes this button from the static group map.
void RadioButton::unregisterGroup() {
    if (group_.empty()) return;
    auto it = s_groups.find(group_);
    if (it == s_groups.end()) return;
    auto& vec = it->second;
    vec.erase(std::remove(vec.begin(), vec.end(), this), vec.end());
    if (vec.empty()) s_groups.erase(it);
}

// ––––––––[ setGroup ] changes group, re-registers in the static map.
void RadioButton::setGroup(std::string g) noexcept {
    if (group_ == g) return;
    unregisterGroup();
    group_ = std::move(g);
    registerGroup();
}

// ––––––––[ setChecked ] updates state, unchecks siblings in same group, fires callback.
void RadioButton::setChecked(bool c) noexcept {
    if (checked_ == c) return;
    if (c && !group_.empty()) {
        auto it = s_groups.find(group_);
        if (it != s_groups.end()) {
            for (auto* rb : it->second) {
                if (rb != this && rb->checked_) {
                    rb->checked_ = false;
                    if (rb->on_change_) rb->on_change_(false);
                }
            }
        }
    }
    checked_ = c;
    if (on_change_) on_change_(checked_);
}

// ––––––––[ setProperty ] supports "checked", "label", "group", plus base props.
void RadioButton::setProperty(std::string_view name, const YamlNode& value) {
    if (name == "checked") { setChecked(value.asBool()); return; }
    if (name == "label")   { label_ = value.asString(); return; }
    if (name == "group")   { setGroup(value.asString()); return; }
    Widget::setProperty(name, value);
}

// ––––––––[ naturalWidth ] toggle_size + spacing + text width + padding.
float RadioButton::naturalWidth(const Theme& theme, const Window& w) const {
    const float size = textSize_ > 0.0f ? textSize_ : theme.metrics.text_size;
    const float ts   = theme.metrics.toggle_size;
    NVGcontext* vg = w.nvgContext();
    if (vg && !label_.empty()) {
        const std::string spec = font_.empty() ? theme.font_family : font_;
        std::string face;
        const_cast<Window&>(w).prepareFont(vg, spec, face);
        nvgFontSize(vg, size);
        nvgFontFace(vg, face.c_str());
        float tw = nvgTextBounds(vg, 0, 0, label_.c_str(), nullptr, nullptr);
        return ts + 8.0f + tw + theme.metrics.button_padding_x * 2.0f;
    }
    return ts + theme.metrics.button_padding_x * 2.0f;
}

// ––––––––[ naturalHeight ] height needed for indicator + padding.
float RadioButton::naturalHeight(const Theme& theme, const Window&) const {
    const float size = textSize_ > 0.0f ? textSize_ : theme.metrics.text_size;
    const float ts   = theme.metrics.toggle_size;
    return (std::max)(ts, size) + theme.metrics.button_padding_y * 2.0f;
}

// ––––––––[ draw ] renders the radio circle, inner dot, label, and focus ring.
void RadioButton::draw(NVGcontext* vg, const Theme& theme, const Window& w) {
    if (!visible_) return;
    const float rx = bounds_.x, ry = bounds_.y;
    const float rh = bounds_.h;
    const float size = textSize_ > 0.0f ? textSize_ : theme.metrics.text_size;
    const float ts   = theme.metrics.toggle_size;
    const float padX = theme.metrics.button_padding_x;
    const float half = ts * 0.5f;
    const float t_h  = hoverAnim_.value();
    const float t_p  = pressAnim_.value();

    // Circle center
    const float cx = rx + padX + half;
    const float cy = ry + rh * 0.5f;

    const Color c_accent = accentColor(theme);
    const Color c_accent_pressed = accentPressedColor(theme);
    // Outer circle fill
    Color fill_col;
    if (!enabled_) {
        fill_col = theme.colors.disabled;
    } else if (checked_) {
        fill_col = blend3(c_accent, c_accent_pressed,
                          c_accent_pressed, t_h, t_p);
    } else {
        fill_col = blend3(theme.colors.surface, c_accent,
                          c_accent_pressed, t_h * 0.3f, t_p * 0.5f);
    }
    nvgBeginPath(vg);
    nvgCircle(vg, cx, cy, half);
    nvgFillColor(vg, nvgRGBA(fill_col.r, fill_col.g, fill_col.b, fill_col.a));
    nvgFill(vg);

    // Outer circle border
    nvgStrokeWidth(vg, theme.metrics.border_width);
    Color border_col = focused_ ? accentColor(theme) : theme.colors.border;
    nvgStrokeColor(vg, nvgRGBA(border_col.r, border_col.g, border_col.b, border_col.a));
    nvgStroke(vg);

    // Inner dot when checked
    if (checked_) {
        nvgBeginPath(vg);
        nvgCircle(vg, cx, cy, half * 0.4f);
        nvgFillColor(vg, nvgRGBA(255, 255, 255, 220));
        nvgFill(vg);
    }

    // Focus ring
    if (focused_) {
        drawFocusRing(vg, bounds_, theme);
    }

    // Label text
    if (!label_.empty()) {
        const std::string spec = font_.empty() ? theme.font_family : font_;
        std::string face;
        w.prepareFont(vg, spec, face);
        nvgFontSize(vg, size);
        nvgFontFace(vg, face.c_str());
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        const float lx = cx + half + 8.0f;
        const float ly = cy;
        nvgFillColor(vg, nvgRGBA(theme.colors.on_surface.r,
                                  theme.colors.on_surface.g,
                                  theme.colors.on_surface.b,
                                  enabled_ ? theme.colors.on_surface.a : 128));
        nvgText(vg, lx, ly, label_.c_str(), nullptr);
    }
}

// ––––––––[ updateHover ] tracks hover state for animation.
void RadioButton::updateHover(float mx, float my) noexcept {
    hovered_ = containsPoint(mx, my);
}

// ––––––––[ tick ] drives hover/press animations.
void RadioButton::tick(float dt, const Theme& theme) noexcept {
    hoverAnim_.setTarget(hovered_ ? 1.0f : 0.0f);
    pressAnim_.setTarget(pressed_ ? 1.0f : 0.0f);
    hoverAnim_.tick(dt, theme.metrics.animation.hover_tau);
    pressAnim_.tick(dt, theme.metrics.animation.press_tau);
}

// ––––––––[ handleEvent ] mouse input: toggle on click, consume events.
EventResult RadioButton::handleEvent(const Event& e, const Window& w) {
    if (!enabled_) return EventResult::Ignored;
    float mx{}, my{};
    w.cursorPosition(mx, my);
    const bool inside = containsPoint(mx, my);
    switch (kind_of(e)) {
    case EventKind::MouseMove:
        return inside ? EventResult::Consumed : EventResult::Ignored;
    case EventKind::MouseDown:
        if (inside) { pressed_ = true; return EventResult::Consumed; }
        return EventResult::Ignored;
    case EventKind::MouseUp: {
        const bool was_pressed = pressed_;
        pressed_ = false;
        if (was_pressed && inside) {
            setChecked(true);
            return EventResult::Consumed;
        }
        return inside ? EventResult::Consumed : EventResult::Ignored;
    }
    default:
        return EventResult::Ignored;
    }
}

} // namespace fluffy
