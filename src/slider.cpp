// src/slider.cpp

// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

// Slider — horizontal draggable track with thumb and animated hover.

#include <fluffy/slider.hpp>
#include <fluffy/window.hpp>
#include <fluffy/yaml_fwd.hpp>
#include <nanovg.h>
#include <algorithm>
#include <cmath>
#include <format>
namespace fluffy {

// ––––––––[ Slider ] constructs with bounds and optional initial value.
Slider::Slider(Rect bounds, float initial)
    : Widget(bounds),
      value_(initial)
{}

// ––––––––[ setValue ] clamps within [min_, max_] and fires change callback.
void Slider::setValue(float v) noexcept {
    v = (std::max)(min_, (std::min)(max_, v));
    if (value_ == v) return;
    value_ = v;
    if (on_change_) on_change_(value_);
}

// ––––––––[ setRange ] sets min and max, clamps current value.
void Slider::setRange(float minVal, float maxVal) noexcept {
    min_ = minVal;
    max_ = maxVal;
    if (value_ < min_) setValue(min_);
    if (value_ > max_) setValue(max_);
}

// ––––––––[ setProperty ] supports "value", "min", "max", "show_value", plus base props.
void Slider::setProperty(std::string_view name, const YamlNode& value) {
    if (name == "value")     { setValue(value.asFloat()); return; }
    if (name == "min")       { min_ = value.asFloat(); return; }
    if (name == "max")       { max_ = value.asFloat(); return; }
    if (name == "show_value"){ show_value_ = value.asBool(); return; }
    if (name == "value_format"){ value_format_ = value.asString(); return; }
    Widget::setProperty(name, value);
}

// ––––––––[ naturalWidth ] reasonable minimum width.
float Slider::naturalWidth(const Theme&, const Window& /*w*/) const {
    return 120.0f;
}

// ––––––––[ naturalHeight ] tall enough for thumb + padding.
float Slider::naturalHeight(const Theme& theme, const Window&) const {
    return theme.metrics.slider_thumb_size + theme.metrics.button_padding_y * 2.0f;
}

// ––––––––[ draw ] renders track, filled portion, thumb, and focus ring.
void Slider::draw(NVGcontext* vg, const Theme& theme, const Window& w) {
    if (!visible_) return;
    const float rx = bounds_.x, ry = bounds_.y;
    const float rw = bounds_.w, rh = bounds_.h;
    const float thumb_r = theme.metrics.slider_thumb_size * 0.5f;
    const float track_h = theme.metrics.slider_track_height;
    const float t_h  = hoverAnim_.value();

    // Track geometry — inset by thumb radius so the thumb stays inside bounds
    const float track_l = rx + thumb_r;
    const float track_r = rx + rw - thumb_r;
    const float track_w = track_r - track_l;
    const float track_y = ry + rh * 0.5f - track_h * 0.5f;

    // Normalised value from [min_, max_] -> [0, 1]
    const float range = max_ - min_;
    const float norm  = range > 0.0f ? (value_ - min_) / range : 0.0f;
    const float thumbX = track_l + norm * track_w;

    const Color c_accent = accentColor(theme);
    const Color c_accent_pressed = accentPressedColor(theme);

    // Track background
    nvgBeginPath(vg);
    nvgRoundedRect(vg, track_l, track_y, track_w, track_h, track_h * 0.5f);
    Color track_bg = hovered_ ? mix(theme.colors.surface, c_accent, 0.15f)
                              : theme.colors.surface;
    nvgFillColor(vg, nvgRGBA(track_bg.r, track_bg.g, track_bg.b, track_bg.a));
    nvgFill(vg);

    // Track border
    nvgStrokeWidth(vg, theme.metrics.border_width);
    Color track_border = focused_ ? c_accent : theme.colors.border;
    nvgStrokeColor(vg, nvgRGBA(track_border.r, track_border.g, track_border.b, track_border.a));
    nvgStroke(vg);

    // Filled portion (left of thumb)
    if (norm > 0.0f) {
        const float fill_w = (std::max)(track_h, norm * track_w);
        nvgBeginPath(vg);
        nvgRoundedRect(vg, track_l, track_y, fill_w, track_h, track_h * 0.5f);
        Color fill_col = mix(c_accent, c_accent_pressed,
                             std::abs(norm - 0.5f) * 0.3f);
        nvgFillColor(vg, nvgRGBA(fill_col.r, fill_col.g, fill_col.b, fill_col.a));
        nvgFill(vg);
    }

    // Thumb circle
    const float thumb_anim_r = thumb_r + t_h * 2.0f;
    const float cy = ry + rh * 0.5f;

    // Thumb shadow (radial gradient matching the button's box-gradient shadow style)
    const Shadow& sh = theme.metrics.shadow;
    if (sh.blur > 0.0f && sh.color.a > 0) {
        const float sx = thumbX + sh.offset_x;
        const float sy = cy + sh.offset_y;
        const float ext = sh.blur;
        NVGcolor sh_inner = nvgRGBA(sh.color.r, sh.color.g, sh.color.b, sh.color.a);
        NVGcolor sh_outer = nvgRGBA(sh.color.r, sh.color.g, sh.color.b, 0);
        NVGpaint sh_paint = nvgRadialGradient(vg, sx, sy, 0.0f, thumb_anim_r + ext,
                                              sh_inner, sh_outer);
        nvgBeginPath(vg);
        nvgCircle(vg, sx, sy, thumb_anim_r + ext);
        nvgFillPaint(vg, sh_paint);
        nvgFill(vg);
    }

    nvgBeginPath(vg);
    nvgCircle(vg, thumbX, cy, thumb_anim_r);
    nvgFillColor(vg, nvgRGBA(theme.colors.surface.r, theme.colors.surface.g,
                              theme.colors.surface.b, theme.colors.surface.a));
    nvgFill(vg);
    // Thumb border
    nvgStrokeWidth(vg, 1.5f);
    nvgStrokeColor(vg, nvgRGBA(c_accent.r, c_accent.g,
                               c_accent.b, c_accent.a));
    nvgStroke(vg);

    // Value label above the thumb
    if (show_value_) {
        const float label_size = theme.metrics.text_size - 2.0f;
        const std::string spec = font_.empty() ? theme.font_family : font_;
        std::string face;
        w.prepareFont(vg, spec, face);
        nvgFontSize(vg, label_size);
        nvgFontFace(vg, face.c_str());
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_BOTTOM);
        const float display = range > 0.0f ? value_ : 0.0f;
        const std::string label = std::vformat(value_format_, std::make_format_args(display));
        const float label_y = cy - thumb_anim_r - 4.0f;
        nvgFillColor(vg, nvgRGBA(theme.colors.on_surface.r,
                                  theme.colors.on_surface.g,
                                  theme.colors.on_surface.b,
                                  theme.colors.on_surface.a));
        nvgText(vg, thumbX, label_y, label.c_str(), nullptr);
    }

    // Focus ring
    if (focused_) {
        drawFocusRing(vg, bounds_, theme);
    }
}

// ––––––––[ updateHover ] tracks hover state for animation.
void Slider::updateHover(float mx, float my) noexcept {
    hovered_ = containsPoint(mx, my);
}

// ––––––––[ tick ] drives hover animation.
void Slider::tick(float dt, const Theme& theme) noexcept {
    hoverAnim_.setTarget(hovered_ ? 1.0f : 0.0f);
    hoverAnim_.tick(dt, theme.metrics.animation.hover_tau);
}

// ––––––––[ handleEvent ] mouse drag on thumb updates value continuously.
EventResult Slider::handleEvent(const Event& e, const Window& w) {
    if (!enabled_) return EventResult::Ignored;
    float mx{}, my{};
    w.cursorPosition(mx, my);
    const bool inside = containsPoint(mx, my);

    switch (kind_of(e)) {
    case EventKind::MouseMove:
        if (dragging_) {
            const float thumb_r = w.theme().metrics.slider_thumb_size * 0.5f;
            const float track_l = bounds_.x + thumb_r;
            const float track_r = bounds_.x + bounds_.w - thumb_r;
            const float track_w = track_r - track_l;
            const float t = (std::max)(0.0f, (std::min)(1.0f, (mx - track_l) / track_w));
            const float range = max_ - min_;
            setValue(min_ + t * range);
            return EventResult::Consumed;
        }
        return inside ? EventResult::Consumed : EventResult::Ignored;
    case EventKind::MouseDown:
        if (inside) {
            dragging_ = true;
            const float thumb_r = w.theme().metrics.slider_thumb_size * 0.5f;
            const float track_l = bounds_.x + thumb_r;
            const float track_r = bounds_.x + bounds_.w - thumb_r;
            const float track_w = track_r - track_l;
            const float t = (std::max)(0.0f, (std::min)(1.0f, (mx - track_l) / track_w));
            const float range = max_ - min_;
            const float clicked_val = min_ + t * range;
            setValue(clicked_val);
            return EventResult::Consumed;
        }
        return EventResult::Ignored;
    case EventKind::MouseUp:
        dragging_ = false;
        return inside ? EventResult::Consumed : EventResult::Ignored;
    default:
        return EventResult::Ignored;
    }
}

} // namespace fluffy
