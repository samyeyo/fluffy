// src/switch.cpp

// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

// Switch — iOS-style pill toggle with sliding thumb and optional label.

#include <fluffy/switch.hpp>
#include <fluffy/window.hpp>
#include <fluffy/yaml_fwd.hpp>
#include <nanovg.h>
namespace fluffy {

// ––––––––[ Switch ] constructs with bounds and optional label string.
Switch::Switch(Rect bounds, std::string label)
    : Widget(bounds),
      label_(std::move(label))
{}

// ––––––––[ setChecked ] updates state and fires change callback.
void Switch::setChecked(bool c) noexcept {
    if (checked_ == c) return;
    checked_ = c;
    if (on_change_) on_change_(checked_);
}

// ––––––––[ setProperty ] supports "checked", "label", plus base props.
void Switch::setProperty(std::string_view name, const YamlNode& value) {
    if (name == "checked") { setChecked(value.asBool()); return; }
    if (name == "label")   { label_ = value.asString(); return; }
    Widget::setProperty(name, value);
}

// ––––––––[ naturalWidth ] track width + spacing + text width + padding.
float Switch::naturalWidth(const Theme& theme, const Window& w) const {
    const float size = textSize_ > 0.0f ? textSize_ : theme.metrics.text_size;
    const float ts   = theme.metrics.toggle_size;
    // Track is about 2.6x toggle_size wide (iOS-style pill)
    const float track_w = ts * 2.6f;
    NVGcontext* vg = w.nvgContext();
    if (vg && !label_.empty()) {
        const std::string spec = font_.empty() ? theme.font_family : font_;
        std::string face;
        const_cast<Window&>(w).prepareFont(vg, spec, face);
        nvgFontSize(vg, size);
        nvgFontFace(vg, face.c_str());
        float tw = nvgTextBounds(vg, 0, 0, label_.c_str(), nullptr, nullptr);
        return track_w + 8.0f + tw + theme.metrics.button_padding_x * 2.0f;
    }
    return track_w + theme.metrics.button_padding_x * 2.0f;
}

// ––––––––[ naturalHeight ] height for thumb + padding.
float Switch::naturalHeight(const Theme& theme, const Window&) const {
    const float size = textSize_ > 0.0f ? textSize_ : theme.metrics.text_size;
    const float ts   = theme.metrics.toggle_size;
    return (std::max)(ts, size) + theme.metrics.button_padding_y * 2.0f;
}

// ––––––––[ draw ] renders the pill track, sliding thumb, label, and focus ring.
void Switch::draw(NVGcontext* vg, const Theme& theme, const Window& w) {
    if (!visible_) return;
    const float rx = bounds_.x, ry = bounds_.y;
    const float rh = bounds_.h;
    const float size = textSize_ > 0.0f ? textSize_ : theme.metrics.text_size;
    const float ts   = theme.metrics.toggle_size;
    const float padX = theme.metrics.button_padding_x;
    const float t_h  = hoverAnim_.value();
    const float t_p  = pressAnim_.value();

    // Track dimensions — pill shape
    const float track_h = ts * 1.2f;
    const float track_w = ts * 2.6f;
    const float track_y = ry + (rh - track_h) * 0.5f;
    const float track_x = rx + padX;
    const float track_r = track_h * 0.5f;

    // Thumb circle — slightly smaller for a clean look
    const float thumb_r = ts * 0.45f;
    const float thumb_min_x = track_x + thumb_r + 3.0f;
    const float thumb_max_x = track_x + track_w - thumb_r - 3.0f;

    // Animated thumb position: 0 at min_x (off), 1 at max_x (on)
    const float thumb_t = thumbAnim_.value();
    const float thumb_x = thumb_min_x + thumb_t * (thumb_max_x - thumb_min_x);
    const float thumb_y = ry + rh * 0.5f;
    const float thumb_anim_r = thumb_r + t_h * 1.5f;

    const Color c_accent = accentColor(theme);
    const Color c_accent_pressed = accentPressedColor(theme);

    // Track — no border (Fluent UI style)
    nvgBeginPath(vg);
    nvgRoundedRect(vg, track_x, track_y, track_w, track_h, track_r);
    Color track_col;
    if (!enabled_) {
        track_col = theme.colors.disabled;
    } else if (checked_) {
        track_col = mix(c_accent, c_accent_pressed, t_p);
    } else {
        track_col = blend3(Color::fromHex(0xFFD1D1D6), c_accent,
                           c_accent_pressed, t_h * 0.2f, t_p * 0.3f);
    }
    nvgFillColor(vg, nvgRGBA(track_col.r, track_col.g, track_col.b, track_col.a));
    nvgFill(vg);

    // Thumb shadow — subtle, from theme
    const Shadow& sh = theme.metrics.shadow;
    if (sh.blur > 0.0f && sh.color.a > 0) {
        const float sx = thumb_x + sh.offset_x;
        const float sy = thumb_y + sh.offset_y;
        NVGcolor sh_inner = nvgRGBA(sh.color.r, sh.color.g, sh.color.b, sh.color.a);
        NVGcolor sh_outer = nvgRGBA(sh.color.r, sh.color.g, sh.color.b, 0);
        NVGpaint sh_paint = nvgRadialGradient(vg, sx, sy, 0.0f,
                                              thumb_anim_r + sh.blur,
                                              sh_inner, sh_outer);
        nvgBeginPath(vg);
        nvgCircle(vg, sx, sy, thumb_anim_r + sh.blur);
        nvgFillPaint(vg, sh_paint);
        nvgFill(vg);
    }

    // Thumb — pure white, no stroke (Fluent UI style)
    nvgBeginPath(vg);
    nvgCircle(vg, thumb_x, thumb_y, thumb_anim_r);
    nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
    nvgFill(vg);

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
        const float lx = track_x + track_w + 8.0f;
        const float ly = ry + rh * 0.5f;
        nvgFillColor(vg, nvgRGBA(theme.colors.on_surface.r,
                                  theme.colors.on_surface.g,
                                  theme.colors.on_surface.b,
                                  enabled_ ? theme.colors.on_surface.a : 128));
        nvgText(vg, lx, ly, label_.c_str(), nullptr);
    }
}

// ––––––––[ updateHover ] tracks hover state for animation.
void Switch::updateHover(float mx, float my) noexcept {
    hovered_ = containsPoint(mx, my);
}

// ––––––––[ tick ] drives hover, press, and thumb slide animations.
void Switch::tick(float dt, const Theme& theme) noexcept {
    hoverAnim_.setTarget(hovered_ ? 1.0f : 0.0f);
    pressAnim_.setTarget(pressed_ ? 1.0f : 0.0f);
    thumbAnim_.setTarget(checked_ ? 1.0f : 0.0f);
    hoverAnim_.tick(dt, theme.metrics.animation.hover_tau);
    pressAnim_.tick(dt, theme.metrics.animation.press_tau);
    thumbAnim_.tick(dt, theme.metrics.animation.hover_tau * 0.6f);
}

// ––––––––[ handleEvent ] mouse input: toggle on click, consume events.
EventResult Switch::handleEvent(const Event& e, const Window& w) {
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
