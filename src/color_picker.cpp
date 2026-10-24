// src/color_picker.cpp
// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
// ColorPicker — HSV bars + editable RGBA/hex fields inside a rounded card.
//
// MIT License - Copyright (c) 2026 - Samir Tine
// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

#include <fluffy/color_picker.hpp>

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstdio>
#include <string>

#include <nanovg.h>

#include <fluffy/geom.hpp>
#include <fluffy/window.hpp>
#include <fluffy/yaml_fwd.hpp>

namespace fluffy {

// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
// ColorPicker
// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

static constexpr float kPad    = 8.0f;
static constexpr float kFieldH = 22.0f;
static constexpr float kBarH   = 18.0f;

ColorPicker::ColorPicker(Rect bounds, Color initial)
    : Widget(bounds), color_(initial)
{
    syncHSV();
    updateHexFromColor();
}

void ColorPicker::setColor(Color c) noexcept {
    if (color_.r == c.r && color_.g == c.g && color_.b == c.b && color_.a == c.a)
        return;
    color_ = c;
    syncHSV();
    updateHexFromColor();
    if (on_change_) on_change_(color_);
}

void ColorPicker::syncHSV() noexcept {
    rgbToHsv(color_, hue_, sat_, val_);
}

void ColorPicker::updateHexFromColor() noexcept {
    std::snprintf(hex_buf_, sizeof(hex_buf_), "%02X%02X%02X%02X",
                  color_.r, color_.g, color_.b, color_.a);
}

void ColorPicker::setFocused(bool f) noexcept {
    if (!f && edit_field_ >= 0) commitEdit();
    focused_ = f;
}

void ColorPicker::commitEdit() noexcept {
    if (edit_field_ < 0) return;
    Color c = color_;
    if (edit_field_ == 4) {
        // Hex field: parse RRGGBB, keep alpha
        if (edit_buf_.size() == 6) {
            auto opt = Color::fromString("#" + edit_buf_);
            if (opt) { c.r = opt->r; c.g = opt->g; c.b = opt->b; }
        }
    } else if (edit_field_ >= 0 && edit_field_ < 4 && !edit_buf_.empty()) {
        int val = -1;
        std::from_chars(edit_buf_.data(), edit_buf_.data() + edit_buf_.size(), val);
        if (val >= 0 && val <= 255) {
            auto v = static_cast<std::uint8_t>(val);
            if      (edit_field_ == 0) c.r = v;
            else if (edit_field_ == 1) c.g = v;
            else if (edit_field_ == 2) c.b = v;
            else if (edit_field_ == 3) c.a = v;
        }
    }
    edit_field_ = -1;
    edit_buf_.clear();
    setColor(c);
}

// ––––––––[ setProperty ] supports "color" as hex string.
void ColorPicker::setProperty(std::string_view name, const YamlNode& value) {
    if (name == "color") {
        auto c = Color::fromString(value.asString());
        if (c) setColor(*c);
        return;
    }
    Widget::setProperty(name, value);
}

// ––––––––[ naturalWidth ] roughly 260px for compact picker with color box.
float ColorPicker::naturalWidth(const Theme&, const Window&) const {
    return 260.0f;
}

// ––––––––[ naturalHeight ]
float ColorPicker::naturalHeight(const Theme&, const Window&) const {
    return 2.0f * kPad + 4.0f * kFieldH + 3.0f * 4.0f;
}

// ––––––––[ fieldRect ] returns hit-test rect for RGBA (0-3) or hex (4) field.
Rect ColorPicker::fieldRect(int field) const noexcept {
    const float preview_l = bounds_.x + kPad;
    const float hex_w     = 56.0f;
    const float col2_x    = preview_l + hex_w + kPad;
    const float col3_x    = col2_x + 72.0f + kPad;
    const float field_w   = 72.0f;
    const float row_y     = bounds_.y + kPad + static_cast<float>(field) * (kFieldH + 4.0f);
    const float hex_y     = bounds_.y + kPad + 65.0f;             // under preview (centered in rows 0-2)
    const float row3_y    = bounds_.y + kPad + 3.0f * (kFieldH + 4.0f);
    if (field == 4) {
        // Hex field: just under preview
        return Rect{preview_l, hex_y, hex_w, kFieldH};
    }
    if (field == 3) {
        // A field: row 3, right of hex
        const float a_l = preview_l + hex_w + kPad;
        const float a_w = col3_x - kPad - a_l;
        return Rect{a_l, row3_y, (std::max)(a_w, 1.0f), kFieldH};
    }
    // R, G, B: rows 0-2
    return Rect{col2_x, row_y, field_w, kFieldH};
}

// ––––––––[ draw ] renders the card, HSV/A bars, RGBA fields, hex, and preview.
void ColorPicker::draw(NVGcontext* vg, const Theme& theme, const Window& w) {
    if (!visible_) return;
    const float rx = bounds_.x, ry = bounds_.y;
    const float rw = bounds_.w, rh = bounds_.h;
    const float corner = 6.0f;
    const std::string spec = font_.empty() ? theme.font_family : font_;
    std::string face;
    w.prepareFont(vg, spec, face);

    // –– Card background + shadow + border ––
    nvgBeginPath(vg);
    nvgRoundedRect(vg, rx, ry, rw, rh, corner);
    nvgFillColor(vg, nvgRGBA(theme.colors.surface.r, theme.colors.surface.g,
                              theme.colors.surface.b, theme.colors.surface.a));
    nvgFill(vg);
    // Card shadow
    const Shadow& sh = theme.metrics.shadow;
    if (sh.blur > 0.0f && sh.color.a > 0) {
        const float sx = rx + sh.offset_x, sy = ry + sh.offset_y;
        NVGcolor sh_inner = nvgRGBA(sh.color.r, sh.color.g, sh.color.b, sh.color.a);
        NVGcolor sh_outer = nvgRGBA(sh.color.r, sh.color.g, sh.color.b, 0);
        NVGpaint sh_paint = nvgRadialGradient(vg, sx, sy, 0.0f, corner + sh.blur, sh_inner, sh_outer);
        nvgBeginPath(vg);
        nvgRoundedRect(vg, rx, ry, rw, rh, corner);
        nvgFillPaint(vg, sh_paint);
        nvgFill(vg);
        nvgBeginPath(vg);
        nvgRoundedRect(vg, rx, ry, rw, rh, corner);
        nvgFillColor(vg, nvgRGBA(theme.colors.surface.r, theme.colors.surface.g,
                                  theme.colors.surface.b, theme.colors.surface.a));
        nvgFill(vg);
    }
    nvgStrokeWidth(vg, 1.0f);
    nvgStrokeColor(vg, nvgRGBA(theme.colors.border.r, theme.colors.border.g,
                                theme.colors.border.b, 120));
    nvgStroke(vg);

    // –– Layout positions ––
    const float preview_w  = 56.0f;
    const float preview_l  = rx + kPad;
    const float preview_h  = 56.0f;                              // square swatch, width matches hex field
    const float preview_t  = ry + kPad + 9.0f;                    // vertically centered in 3 field rows
    const float hex_w      = 56.0f;
    const float col2_x     = preview_l + hex_w + kPad;           // all fields (R, G, B, A) left-aligned
    const float col3_x     = col2_x + 72.0f + kPad;            // bars column (all rows)
    const float bar_label_w = 14.0f;
    const float bar_track_l = col3_x + bar_label_w + 4.0f;
    const float bar_track_r = rx + rw - kPad;
    const float bar_track_w = bar_track_r - bar_track_l;

    // Helper lambdas
    auto drawCheckerboard = [&](float l, float t, float w, float h) {
        const float cs = 6.0f;
        for (float cy = t; cy < t + h; cy += cs) {
            for (float cx = l; cx < l + w; cx += cs) {
                const int idx = static_cast<int>((cx - l) / cs) + static_cast<int>((cy - t) / cs);
                const auto b = static_cast<std::uint8_t>((idx & 1) ? 230 : 190);
                nvgBeginPath(vg);
                nvgRect(vg, cx, cy, cs, cs);
                nvgFillColor(vg, nvgRGBA(b, b, b, 255));
                nvgFill(vg);
            }
        }
    };

    auto drawBarMarker = [&](float cx, float cy, bool active) {
        nvgBeginPath(vg);
        nvgCircle(vg, cx, cy, 6.0f);
        nvgFillColor(vg, nvgRGBA(255, 255, 255, 240));
        nvgFill(vg);
        nvgStrokeWidth(vg, 1.5f);
        nvgStrokeColor(vg, nvgRGBA(60, 60, 60, active ? 200 : 120));
        nvgStroke(vg);
    };

    auto drawFieldBg = [&](float fx, float fy, float fw, float fh, bool editing) {
        nvgBeginPath(vg);
        nvgRoundedRect(vg, fx, fy, fw, fh, 4.0f);
        if (editing) {
            nvgFillColor(vg, nvgRGBA(theme.colors.accent.r, theme.colors.accent.g,
                                      theme.colors.accent.b, 40));
        } else {
            nvgFillColor(vg, nvgRGBA(theme.colors.border.r, theme.colors.border.g,
                                      theme.colors.border.b, 50));
        }
        nvgFill(vg);
    };

    // –– Preview swatch (with checkerboard for alpha) ––
    nvgSave(vg);
    nvgIntersectScissor(vg, preview_l, preview_t, preview_w, preview_h);
    drawCheckerboard(preview_l, preview_t, preview_w, preview_h);
    nvgRestore(vg);
    {
        nvgBeginPath(vg);
        nvgRect(vg, preview_l, preview_t, preview_w, preview_h);
        nvgFillColor(vg, nvgRGBA(color_.r, color_.g, color_.b, color_.a));
        nvgFill(vg);
        nvgStrokeWidth(vg, 1.0f);
        nvgStrokeColor(vg, nvgRGBA(theme.colors.border.r, theme.colors.border.g,
                                    theme.colors.border.b, 180));
        nvgStroke(vg);
    }

    // –– RGBA fields (R/G row1, B/A row2) ––
    const char* field_labels = "RGBA";
    static constexpr std::uint8_t comp_colors[4][3] = {
        {235, 70, 70}, {70, 200, 70}, {70, 110, 240}, {160, 160, 160}
    };
    const std::uint8_t comp_vals[4] = {color_.r, color_.g, color_.b, color_.a};
    nvgFontSize(vg, 11.0f);
    nvgFontFace(vg, face.c_str());

    for (int fi = 0; fi < 4; ++fi) {
        const Rect fr = fieldRect(fi);
        const bool editing = (edit_field_ == fi);
        drawFieldBg(fr.x, fr.y, fr.w, fr.h, editing);

        // Label character
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, nvgRGBA(comp_colors[fi][0], comp_colors[fi][1],
                                  comp_colors[fi][2], 230));
        char lbl[2] = {field_labels[fi], '\0'};
        nvgText(vg, fr.x + 4.0f, fr.y + fr.h * 0.5f, lbl, nullptr);

        // Value text
        std::string val_str;
        if (editing) {
            if (edit_buf_.empty()) {
                val_str = std::to_string(comp_vals[fi]);
            } else {
                val_str = edit_buf_;
            }
        } else {
            val_str = std::to_string(comp_vals[fi]);
        }
        nvgTextAlign(vg, NVG_ALIGN_RIGHT | NVG_ALIGN_MIDDLE);
        const bool hint = editing && edit_buf_.empty();
        nvgFillColor(vg, nvgRGBA(theme.colors.on_surface.r, theme.colors.on_surface.g,
                                  theme.colors.on_surface.b, hint ? 100 : editing ? 255 : 200));
        nvgText(vg, fr.x + fr.w - 4.0f, fr.y + fr.h * 0.5f, val_str.c_str(), nullptr);

        // Cursor when editing — at right edge (right-aligned text extends leftward)
        if (editing) {
            const float cursor_x = fr.x + fr.w - 4.0f + 2.0f;
            nvgBeginPath(vg);
            nvgRect(vg, cursor_x, fr.y + 4.0f, 1.5f, fr.h - 8.0f);
            nvgFillColor(vg, nvgRGBA(theme.colors.on_surface.r, theme.colors.on_surface.g,
                                      theme.colors.on_surface.b, 220));
            nvgFill(vg);
        }
    }

    // –– Hex field (under preview) ––
    {
        const Rect hr = fieldRect(4);
        const bool editing = (edit_field_ == 4);
        drawFieldBg(hr.x, hr.y, hr.w, hr.h, editing);

        nvgFontSize(vg, 11.0f);
        nvgFontFace(vg, face.c_str());
        std::string hex_str = "#";
        if (editing) {
            if (edit_buf_.empty()) {
                hex_str += std::string(hex_buf_, 6);
            } else {
                hex_str += edit_buf_;
            }
        } else {
            hex_str += std::string(hex_buf_, 6);
        }
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        const bool hex_hint = editing && edit_buf_.empty();
        nvgFillColor(vg, nvgRGBA(theme.colors.on_surface.r, theme.colors.on_surface.g,
                                  theme.colors.on_surface.b, hex_hint ? 100 : editing ? 255 : 160));
        nvgText(vg, hr.x + 6.0f, hr.y + hr.h * 0.5f, hex_str.c_str(), nullptr);

        if (editing) {
            float bnds[4];
            nvgTextBounds(vg, 0, 0, hex_str.c_str(), nullptr, bnds);
            float cursor_x = hr.x + 6.0f + (bnds[2] - bnds[0]) + 1.0f;
            nvgBeginPath(vg);
            nvgRect(vg, cursor_x, hr.y + 4.0f, 1.5f, hr.h - 8.0f);
            nvgFillColor(vg, nvgRGBA(theme.colors.on_surface.r, theme.colors.on_surface.g,
                                      theme.colors.on_surface.b, 220));
            nvgFill(vg);
        }


    }

    // –– HSV/A bars (same rows as RGBA fields) ––
    const char bar_labels[4] = {'H', 'S', 'V', 'A'};
    for (int bi = 0; bi < 4; ++bi) {
        const float row_y  = ry + kPad + static_cast<float>(bi) * (kFieldH + 4.0f);
        const float bar_cy = row_y + kFieldH * 0.5f;
        const float bar_top = bar_cy - kBarH * 0.5f;

        // Label
        nvgFontSize(vg, 10.0f);
        nvgFontFace(vg, face.c_str());
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, nvgRGBA(theme.colors.on_surface.r, theme.colors.on_surface.g,
                                  theme.colors.on_surface.b, 180));
        char blbl[2] = {bar_labels[bi], '\0'};
        nvgText(vg, col3_x, bar_cy, blbl, nullptr);

        // Track background
        const float track_bot = bar_top;
        const float track_h   = kBarH;

        if (bi == 0) {
            // Hue bar: 6 rainbow segments with rounded outer corners
            const float seg_w = bar_track_w / 6.0f;
            const float r     = track_h * 0.5f;
            for (int seg = 0; seg < 6; ++seg) {
                const float a = static_cast<float>(seg) / 6.0f;
                const float b = static_cast<float>(seg + 1) / 6.0f;
                const Color cl = hsvToRgb(a * 360.0f, 1.0f, 1.0f);
                const Color cr = hsvToRgb(b * 360.0f, 1.0f, 1.0f);
                const float sl = bar_track_l + seg_w * static_cast<float>(seg);
                NVGpaint grad = nvgLinearGradient(vg, sl, 0, sl + seg_w, 0,
                    nvgRGBA(cl.r, cl.g, cl.b, 255),
                    nvgRGBA(cr.r, cr.g, cr.b, 255));
                nvgBeginPath(vg);
                if (seg == 0) {
                    nvgRoundedRectVarying(vg, sl, track_bot, seg_w + 0.5f, track_h, r, 0, 0, r);
                } else if (seg == 5) {
                    nvgRoundedRectVarying(vg, sl, track_bot, seg_w + 0.5f, track_h, 0, r, r, 0);
                } else {
                    nvgRect(vg, sl, track_bot, seg_w + 0.5f, track_h);
                }
                nvgFillPaint(vg, grad);
                nvgFill(vg);
            }
            // Outline for the hue track
            nvgBeginPath(vg);
            nvgRoundedRect(vg, bar_track_l, track_bot, bar_track_w, track_h, r);
            nvgStrokeWidth(vg, 1.0f);
            nvgStrokeColor(vg, nvgRGBA(0, 0, 0, 80));
            nvgStroke(vg);
        } else if (bi == 1) {
            // Saturation bar: gray → full color
            const Color full = hsvToRgb(hue_, 1.0f, val_);
            const auto gray_v = static_cast<std::uint8_t>(std::clamp(val_ * 255.0f, 0.0f, 255.0f) + 0.5f);
            NVGpaint grad = nvgLinearGradient(vg, bar_track_l, 0, bar_track_r, 0,
                nvgRGBA(gray_v, gray_v, gray_v, 255),
                nvgRGBA(full.r, full.g, full.b, 255));
            nvgBeginPath(vg);
            nvgRoundedRect(vg, bar_track_l, track_bot, bar_track_w, track_h, track_h * 0.5f);
            nvgFillPaint(vg, grad);
            nvgFill(vg);
        } else if (bi == 2) {
            // Value bar: black → full color
            const Color full = hsvToRgb(hue_, sat_, 1.0f);
            NVGpaint grad = nvgLinearGradient(vg, bar_track_l, 0, bar_track_r, 0,
                nvgRGBA(0, 0, 0, 255),
                nvgRGBA(full.r, full.g, full.b, 255));
            nvgBeginPath(vg);
            nvgRoundedRect(vg, bar_track_l, track_bot, bar_track_w, track_h, track_h * 0.5f);
            nvgFillPaint(vg, grad);
            nvgFill(vg);
        } else {
            // Alpha bar: checkerboard + transparent → opaque gradient
            drawCheckerboard(bar_track_l, track_bot, bar_track_w, track_h);
            NVGpaint grad = nvgLinearGradient(vg, bar_track_l, 0, bar_track_r, 0,
                nvgRGBA(color_.r, color_.g, color_.b, 0),
                nvgRGBA(color_.r, color_.g, color_.b, 255));
            nvgBeginPath(vg);
            nvgRoundedRect(vg, bar_track_l, track_bot, bar_track_w, track_h, track_h * 0.5f);
            nvgFillPaint(vg, grad);
            nvgFill(vg);
        }

        // Bar marker (current value position)
        float marker_norm = 0.0f;
        if (bi == 0) marker_norm = hue_ / 360.0f;
        else if (bi == 1) marker_norm = sat_;
        else if (bi == 2) marker_norm = val_;
        else marker_norm = color_.a / 255.0f;

        const float marker_x = bar_track_l + marker_norm * bar_track_w;
        drawBarMarker(marker_x, bar_cy, drag_bar_ == bi);
    }

    // Focus ring
    if (focused_) drawFocusRing(vg, bounds_, theme);
}

// ––––––––[ updateHover ] tracks hover state.
void ColorPicker::updateHover(float mx, float my) noexcept {
    hovered_ = containsPoint(mx, my);
}

// ––––––––[ tick ] drives hover animation and cursor blink.
void ColorPicker::tick(float dt, const Theme& theme) noexcept {
    hoverAnim_.setTarget(hovered_ ? 1.0f : 0.0f);
    hoverAnim_.tick(dt, theme.metrics.animation.hover_tau);
    cursor_t_ += dt;
}

// ––––––––[ handleEvent ] mouse drag + text editing.
EventResult ColorPicker::handleEvent(const Event& e, const Window& w) {
    if (!enabled_) return EventResult::Ignored;
    float mx{}, my{};
    w.cursorPosition(mx, my);

    switch (kind_of(e)) {

    case EventKind::MouseMove:
        if (drag_bar_ >= 0) {
            const float col3_x = bounds_.x + kPad + 56.0f + kPad + 72.0f + kPad;
            const float bar_track_l = col3_x + 14.0f + 4.0f;
            const float bar_track_r = bounds_.x + bounds_.w - kPad;
            const float t = std::clamp((mx - bar_track_l) / (bar_track_r - bar_track_l), 0.0f, 1.0f);
            Color c = color_;
            if (drag_bar_ == 0) {
                hue_ = t * 360.0f;
                Color rgb = hsvToRgb(hue_, sat_, val_);
                c.r = rgb.r; c.g = rgb.g; c.b = rgb.b;
            } else if (drag_bar_ == 1) {
                sat_ = t;
                Color rgb = hsvToRgb(hue_, sat_, val_);
                c.r = rgb.r; c.g = rgb.g; c.b = rgb.b;
            } else if (drag_bar_ == 2) {
                val_ = t;
                Color rgb = hsvToRgb(hue_, sat_, val_);
                c.r = rgb.r; c.g = rgb.g; c.b = rgb.b;
            } else if (drag_bar_ == 3) {
                c.a = static_cast<std::uint8_t>(t * 255.0f + 0.5f);
            }
            setColor(c);
            return EventResult::Consumed;
        }
        return containsPoint(mx, my) ? EventResult::Consumed : EventResult::Ignored;

    case EventKind::MouseDown: {
        if (!containsPoint(mx, my)) return EventResult::Ignored;

        // Check RGBA / hex field clicks
        for (int fi = 0; fi < 5; ++fi) {
            const Rect fr = fieldRect(fi);
            if (mx >= fr.x && mx < fr.x + fr.w && my >= fr.y && my < fr.y + fr.h) {
                // Commit any pending edit first
                if (edit_field_ >= 0 && edit_field_ != fi) commitEdit();
                edit_field_ = fi;
                edit_buf_.clear();
                return EventResult::Consumed;
            }
        }

        // Commit any pending edit when clicking elsewhere on the picker
        if (edit_field_ >= 0) commitEdit();

        // Check bars
        const float col3_x = bounds_.x + kPad + 56.0f + kPad + 72.0f + kPad;
        const float bar_track_l = col3_x + 14.0f + 4.0f;
        const float bar_track_r = bounds_.x + bounds_.w - kPad;

        if (mx >= bar_track_l && mx < bar_track_r) {
            for (int bi = 0; bi < 4; ++bi) {
                const float row_y = bounds_.y + kPad + static_cast<float>(bi) * (kFieldH + 4.0f);
                if (my >= row_y && my < row_y + kFieldH) {
                    drag_bar_ = bi;
                    const float t = std::clamp((mx - bar_track_l) / (bar_track_r - bar_track_l), 0.0f, 1.0f);
                    Color c = color_;
                    if (bi == 0) {
                        hue_ = t * 360.0f;
                        Color rgb = hsvToRgb(hue_, sat_, val_);
                        c.r = rgb.r; c.g = rgb.g; c.b = rgb.b;
                    } else if (bi == 1) {
                        sat_ = t;
                        Color rgb = hsvToRgb(hue_, sat_, val_);
                        c.r = rgb.r; c.g = rgb.g; c.b = rgb.b;
                    } else if (bi == 2) {
                        val_ = t;
                        Color rgb = hsvToRgb(hue_, sat_, val_);
                        c.r = rgb.r; c.g = rgb.g; c.b = rgb.b;
                    } else if (bi == 3) {
                        c.a = static_cast<std::uint8_t>(t * 255.0f + 0.5f);
                    }
                    setColor(c);
                    return EventResult::Consumed;
                }
            }
        }

        return EventResult::Consumed;
    }

    case EventKind::MouseUp:
        if (drag_bar_ >= 0) {
            drag_bar_ = -1;
            return EventResult::Consumed;
        }
        return containsPoint(mx, my) ? EventResult::Consumed : EventResult::Ignored;

    case EventKind::KeyDown: {
        if (edit_field_ < 0) return EventResult::Ignored;
        const auto& ke = std::get<KeyDownEvent>(e);
        if (ke.key == 257) {  // GLFW_KEY_ENTER
            commitEdit();
            return EventResult::Consumed;
        }
        if (ke.key == 256) {  // GLFW_KEY_ESCAPE
            edit_field_ = -1;
            edit_buf_.clear();
            return EventResult::Consumed;
        }
        if (ke.key == 259) {  // GLFW_KEY_BACKSPACE
            if (!edit_buf_.empty()) edit_buf_.pop_back();
            return EventResult::Consumed;
        }
        return EventResult::Consumed;
    }

    case EventKind::Char: {
        if (edit_field_ < 0) return EventResult::Ignored;
        const auto& ce = std::get<CharEvent>(e);
        const char ch = static_cast<char>(ce.codepoint);
        if (edit_field_ == 4) {
            // Hex field: accept 0-9 a-f A-F, max 6 chars
            if (std::isxdigit(static_cast<unsigned char>(ch)) && edit_buf_.size() < 6) {
                edit_buf_.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(ch))));
                // Auto-commit on 6 chars
                if (edit_buf_.size() == 6) commitEdit();
            }
        } else {
            // RGBA field: accept 0-9, max 3 chars
            if (ch >= '0' && ch <= '9' && edit_buf_.size() < 3) {
                edit_buf_.push_back(ch);
                // Auto-commit on 3 chars
                if (edit_buf_.size() == 3) commitEdit();
            }
        }
        return EventResult::Consumed;
    }

    default:
        return EventResult::Ignored;
    }
}

} // namespace fluffy
