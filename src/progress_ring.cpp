// src/progress_ring.cpp
// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
// ProgressRing — draw arc spinner
// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
#include <fluffy/progress_ring.hpp>

#include <algorithm>
#include <cmath>
#include <numbers>

#include <nanovg.h>

#include <fluffy/window.hpp>
#include <fluffy/yaml_fwd.hpp>

namespace fluffy {

// ––––––––[ ProgressRing ] constructs
ProgressRing::ProgressRing(Rect bounds, float value)
    : Widget(bounds), value_(value) {}

// ––––––––[ setValue ] sets
void ProgressRing::setValue(float v) {
    if (v < -0.5f) value_ = -1.0f;
    else value_ = std::clamp(v, 0.0f, 1.0f);
}

// ––––––––[ setIndeterminate ] sets
void ProgressRing::setIndeterminate(bool b) { value_ = b ? -1.0f : 0.0f; }

// ––––––––[ naturalWidth ] fixed 32
float ProgressRing::naturalWidth(const Theme&, const Window&) const { return 32.0f; }

// ––––––––[ naturalHeight ] fixed 32
float ProgressRing::naturalHeight(const Theme&, const Window&) const { return 32.0f; }

// ––––––––[ draw ] arc
void ProgressRing::draw(NVGcontext* vg, const Theme& theme, const Window&) {
    if (!visible_) return;
    float cx = bounds_.x + bounds_.w * 0.5f;
    float cy = bounds_.y + bounds_.h * 0.5f;
    float r = std::min(bounds_.w, bounds_.h) * 0.5f - 2.0f;
    if (r < 4.0f) r = 4.0f;
    // Track
    nvgBeginPath(vg);
    nvgCircle(vg, cx, cy, r);
    nvgStrokeWidth(vg, 3.0f);
    nvgStrokeColor(vg, nvgRGBA(theme.colors.border.r, theme.colors.border.g, theme.colors.border.b, 80));
    nvgStroke(vg);
    if (value_ < 0.0f) {
        // Indeterminate spinner 270deg arc
        float a0 = angle_;
        float a1 = angle_ + 270.0f * std::numbers::pi_v<float> / 180.0f;
        nvgBeginPath(vg);
        nvgArc(vg, cx, cy, r, a0, a1, NVG_CW);
        nvgStrokeWidth(vg, 3.0f);
        nvgStrokeColor(vg, nvgRGBA(theme.colors.accent.r, theme.colors.accent.g, theme.colors.accent.b, theme.colors.accent.a));
        nvgStroke(vg);
        // Cap
        float ex = cx + std::cos(a1) * r;
        float ey = cy + std::sin(a1) * r;
        nvgBeginPath(vg);
        nvgCircle(vg, ex, ey, 1.5f);
        nvgFillColor(vg, nvgRGBA(theme.colors.accent.r, theme.colors.accent.g, theme.colors.accent.b, theme.colors.accent.a));
        nvgFill(vg);
    } else {
        float a0 = -90.0f * std::numbers::pi_v<float> / 180.0f;
        float a1 = a0 + value_ * 360.0f * std::numbers::pi_v<float> / 180.0f;
        if (value_ > 0.001f) {
            nvgBeginPath(vg);
            nvgArc(vg, cx, cy, r, a0, a1, NVG_CW);
            nvgStrokeWidth(vg, 3.0f);
            nvgStrokeColor(vg, nvgRGBA(theme.colors.accent.r, theme.colors.accent.g, theme.colors.accent.b, theme.colors.accent.a));
            nvgStroke(vg);
        }
        // Text percentage
        char buf[16];
        std::snprintf(buf, sizeof(buf), "%d%%",
                      static_cast<int>(std::round(value_ * 100.0f)));
        nvgFontSize(vg, 8.0f);
        // Use default font
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, nvgRGBA(theme.colors.on_surface.r, theme.colors.on_surface.g, theme.colors.on_surface.b, theme.colors.on_surface.a));
        nvgText(vg, cx, cy, buf, nullptr);
    }
}

// ––––––––[ handleEvent ] ignored
EventResult ProgressRing::handleEvent(const Event&, const Window&) { return EventResult::Ignored; }

// ––––––––[ tick ] spinner
void ProgressRing::tick(float dt, const Theme&) noexcept {
    if (value_ < 0.0f) {
        angle_ += dt * 4.0f;
        constexpr float kTau = 2.0f * std::numbers::pi_v<float>;
        if (angle_ > kTau) angle_ -= kTau;
    }
}

// ––––––––[ setProperty ] yaml
void ProgressRing::setProperty(std::string_view name, const YamlNode& value) {
    if (name == "value") { setValue(value.asFloat()); return; }
    if (name == "indeterminate") { setIndeterminate(value.asBool()); return; }
    Widget::setProperty(name, value);
}

}  // namespace fluffy
