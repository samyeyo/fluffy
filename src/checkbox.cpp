// src/checkbox.cpp

// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

// Checkbox — toggleable square indicator with optional text label.

#include <fluffy/checkbox.hpp>
#include <fluffy/window.hpp>
#include <fluffy/yaml_fwd.hpp>
#include <nanovg.h>
#include <algorithm>
namespace fluffy {

// ––––––––[ Checkbox ] constructs with bounds and optional label string.
Checkbox::Checkbox(Rect bounds, std::string label)
    : Widget(bounds),
      label_(std::move(label))
{}

// ––––––––[ setChecked ] updates state and fires change callback.
void Checkbox::setChecked(bool c) noexcept {
    if (checked_ == c) return;
    checked_ = c;
    if (on_change_) on_change_(checked_);
}

// ––––––––[ setProperty ] supports "checked", "label", plus base props.
void Checkbox::setProperty(std::string_view name, const YamlNode& value) {
    if (name == "checked") { setChecked(value.asBool()); return; }
    if (name == "label")   { label_ = value.asString(); return; }
    Widget::setProperty(name, value);
}

// ––––––––[ naturalWidth ] toggle_size + spacing + text width + padding.
float Checkbox::naturalWidth(const Theme& theme, const Window& w) const {
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
float Checkbox::naturalHeight(const Theme& theme, const Window&) const {
    const float size = textSize_ > 0.0f ? textSize_ : theme.metrics.text_size;
    const float ts   = theme.metrics.toggle_size;
    return (std::max)(ts, size) + theme.metrics.button_padding_y * 2.0f;
}

// ––––––––[ draw ] renders the checkbox indicator, checkmark, label, and focus ring.
void Checkbox::draw(NVGcontext* vg, const Theme& theme, const Window& w) {
    if (!visible_) return;
    const float rx = bounds_.x, ry = bounds_.y;
    const float rh = bounds_.h;
    const float size = textSize_ > 0.0f ? textSize_ : theme.metrics.text_size;
    const float ts   = theme.metrics.toggle_size;
    const float padX = theme.metrics.button_padding_x;
    const float t_h  = hoverAnim_.value();
    const float t_p  = pressAnim_.value();

    // Indicator box centered vertically
    const float bx = rx + padX;
    const float by = ry + (rh - ts) * 0.5f;
    const float corner = 3.0f;

    const Color c_accent = accentColor(theme);
    const Color c_accent_pressed = accentPressedColor(theme);
    // Box background fill
    Color box_col;
    if (!enabled_) {
        box_col = theme.colors.disabled;
    } else if (checked_) {
        box_col = blend3(c_accent, c_accent_pressed,
                         c_accent_pressed, t_h, t_p);
    } else {
        box_col = blend3(theme.colors.surface, c_accent,
                         c_accent_pressed, t_h * 0.3f, t_p * 0.5f);
    }
    nvgBeginPath(vg);
    nvgRoundedRect(vg, bx, by, ts, ts, corner);
    nvgFillColor(vg, nvgRGBA(box_col.r, box_col.g, box_col.b, box_col.a));
    nvgFill(vg);

    // Box border
    nvgStrokeWidth(vg, theme.metrics.border_width);
    Color border_col = focused_ ? accentColor(theme) : theme.colors.border;
    nvgStrokeColor(vg, nvgRGBA(border_col.r, border_col.g, border_col.b, border_col.a));
    nvgStroke(vg);

    // Checkmark — V shape from top-left to centre to bottom-right
    if (checked_) {
        nvgBeginPath(vg);
        const float inset = ts * 0.2f;
        nvgMoveTo(vg, bx + inset,          by + ts * 0.55f);
        nvgLineTo(vg, bx + ts * 0.4f,     by + ts * 0.75f);
        nvgLineTo(vg, bx + ts - inset,    by + ts * 0.25f);
        nvgStrokeWidth(vg, 2.0f);
        nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 220));
        nvgStroke(vg);
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
        const float lx = bx + ts + 8.0f;
        const float ly = ry + rh * 0.5f;
        nvgFillColor(vg, nvgRGBA(theme.colors.on_surface.r,
                                  theme.colors.on_surface.g,
                                  theme.colors.on_surface.b,
                                  enabled_ ? theme.colors.on_surface.a : 128));
        nvgText(vg, lx, ly, label_.c_str(), nullptr);
    }
}

// ––––––––[ updateHover ] tracks hover state for animation.
void Checkbox::updateHover(float mx, float my) noexcept {
    hovered_ = containsPoint(mx, my);
}

// ––––––––[ tick ] drives hover/press animations.
void Checkbox::tick(float dt, const Theme& theme) noexcept {
    hoverAnim_.setTarget(hovered_ ? 1.0f : 0.0f);
    pressAnim_.setTarget(pressed_ ? 1.0f : 0.0f);
    hoverAnim_.tick(dt, theme.metrics.animation.hover_tau);
    pressAnim_.tick(dt, theme.metrics.animation.press_tau);
}

// ––––––––[ handleEvent ] mouse input: toggle on click, consume events.
EventResult Checkbox::handleEvent(const Event& e, const Window& w) {
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
            setChecked(!checked_);
            return EventResult::Consumed;
        }
        return inside ? EventResult::Consumed : EventResult::Ignored;
    }
    default:
        return EventResult::Ignored;
    }
}

} // namespace fluffy
