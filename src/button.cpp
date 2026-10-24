// src/button.cpp

// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

// Button — interactive rounded surface with animated hover/press visuals.

#include <fluffy/button.hpp>
#include <fluffy/icons.hpp>
#include <fluffy/inline.hpp>
#include <fluffy/window.hpp>
#include <fluffy/yaml_fwd.hpp>
#include <nanovg.h>
#include <algorithm>
namespace fluffy {
Button::Button(Rect bounds, std::string label)
    : Widget(bounds),
      label_(std::move(label)),
      inline_{},
      is_inline_(false)
{}
Button::Button(Rect bounds, Inline content)
    : Widget(bounds), inline_(std::move(content.runs())), is_inline_(true)
{}
void Button::setLabel(std::string l) {
    label_      = std::move(l);
    inline_.clear();
    is_inline_  = false;
}
void Button::setInline(Inline c) {
    label_.clear();
    inline_      = std::move(c.runs());
    is_inline_   = true;
}
void Button::setProperty(std::string_view name, const YamlNode& value) {
    if (name == "text") { setLabel(value.asString()); return; }
    Widget::setProperty(name, value);
}
float Button::naturalWidth(const Theme& theme, const Window& w) const {
    NVGcontext* vg = w.nvgContext();
    if (!vg) return 0.0f;
    const float size = textSize_ > 0.0f ? textSize_ : theme.metrics.text_size;
    const std::string spec = font_.empty() ? theme.font_family : font_;
    std::string face;
    const_cast<Window&>(w).prepareFont(vg, spec, face);
    nvgFontSize(vg, size);
    nvgFontFace(vg, face.c_str());
    const float iconGain = theme.metrics.animation.icon_size_gain;
    const float iconPad  = theme.metrics.animation.icon_padding;
    float extra = 0.0f;
    if (dropdownMenu_) extra += size * iconGain * 1.8f;  // chevron space
    if (is_inline_) {
        float total = 0.0f;
        for (const auto& r : inline_) {
            if (r.type == Inline::Run::Type::SvgIcon) {
                const float rs = r.size > 0.0f ? r.size : size;
                total += rs * iconGain * (1.0f + 2.0f * iconPad);
            } else {
                total += nvgTextBounds(vg, 0, 0, r.utf8.c_str(), nullptr, nullptr);
            }
        }
        return total + theme.metrics.button_padding_x * 2.0f + extra;
    }
    return nvgTextBounds(vg, 0.0f, 0.0f, label_.c_str(), nullptr, nullptr)
           + theme.metrics.button_padding_x * 2.0f + extra;
}
float Button::naturalHeight(const Theme& theme, const Window&) const {
    const float size = textSize_ > 0.0f ? textSize_ : theme.metrics.text_size;
    return size + theme.metrics.button_padding_y * 2.0f;
}
void Button::draw(NVGcontext* vg, const Theme& theme, const Window& w) {
    if (!visible_) return;
    const float t_h = hoverAnim_.value();
    const float t_p = pressAnim_.value();
    const float press_inset = enabled_ ? theme.metrics.animation.press_inset * t_p : 0.0f;
    const float press_drop  = enabled_ ? theme.metrics.animation.press_drop  * t_p : 0.0f;
    const float rx = bounds_.x + press_inset;
    const float ry = bounds_.y + press_inset + press_drop;
    const float rw = std::max(0.0f, bounds_.w - 2.0f * press_inset);
    const float rh = std::max(0.0f, bounds_.h - 2.0f * press_inset);
    const Shadow& s_rest = theme.metrics.shadow;
    const Shadow& s_hov  = theme.metrics.shadow_hover;
    const Shadow& s_prs  = theme.metrics.shadow_pressed;
    Color shadow_color;
    float shadow_blur, shadow_dx, shadow_dy;
    shadow_color = blend3(s_rest.color, s_hov.color,
                          s_prs.color,       t_h, t_p);
    shadow_blur  = blend3(s_rest.blur,  s_hov.blur,
                          s_prs.blur,    t_h, t_p);
    shadow_dx    = blend3(s_rest.offset_x, s_hov.offset_x,
                          s_prs.offset_x,   t_h, t_p);
    shadow_dy    = blend3(s_rest.offset_y, s_hov.offset_y,
                          s_prs.offset_y,   t_h, t_p);
    if (shadow_blur > 0.0f && shadow_color.a > 0) {
        const float sx = rx + shadow_dx;
        const float sy = ry + shadow_dy;
        const float ext = shadow_blur;
        const NVGcolor shadow_inner = nvgRGBA(
            shadow_color.r, shadow_color.g,
            shadow_color.b, shadow_color.a);
        const NVGcolor shadow_outer = nvgRGBA(
            shadow_color.r, shadow_color.g,
            shadow_color.b, 0);
        NVGpaint shadow_paint = nvgBoxGradient(
            vg,
            sx, sy,
            rw, rh,
            theme.metrics.corner_radius,
            ext,
            shadow_inner,
            shadow_outer);
        nvgBeginPath(vg);
        nvgRect(vg,
                sx - ext, sy - ext,
                rw + 2.0f * ext,
                rh + 2.0f * ext);
        nvgFillPaint(vg, shadow_paint);
        nvgFill(vg);
    }
    const Color c_accent = accentColor(theme);
    const Color c_accent_pressed = accentPressedColor(theme);
    Color fill = blend3(theme.colors.surface,
                        c_accent,
                        c_accent_pressed,
                        t_h, t_p);
    nvgBeginPath(vg);
    nvgRoundedRect(vg, rx, ry, rw, rh, theme.metrics.corner_radius);
    nvgFillColor(vg, nvgRGBA(fill.r, fill.g, fill.b, fill.a));
    nvgFill(vg);
    nvgStrokeWidth(vg, theme.metrics.border_width);
    nvgStrokeColor(vg, nvgRGBA(theme.colors.border.r, theme.colors.border.g,
                               theme.colors.border.b, theme.colors.border.a));
    nvgStroke(vg);
    const float text_t = std::max(t_h, t_p);
    Color text_col;
    if (!enabled_) {
        text_col = theme.colors.on_surface_mute;
    } else {
        text_col = mix(theme.colors.on_surface,
                       theme.colors.surface,
                       text_t);
    }
    // Focus ring
    if (focused_) {
        drawFocusRing(vg, bounds_, theme);
    }
    const float size = textSize_ > 0.0f ? textSize_
                                         : theme.metrics.text_size;
    const std::string spec = font_.empty() ? theme.font_family : font_;
    std::string face;  w.prepareFont(vg, spec, face);
    const float icon_gain  = theme.metrics.animation.icon_size_gain;
    const float icon_pad_t = theme.metrics.animation.icon_padding;
    const Color default_text = text_col;
    const Color default_icon = mix(accentColor(theme), theme.colors.surface, text_t);

    // Text/icon content — shift left slightly if dropdown arrow is shown
    const float chevron_w = dropdownMenu_ ? size * icon_gain * 1.5f : 0.0f;
    const float content_cx = rx + (rw - chevron_w) * 0.5f;

    if (is_inline_) {
        renderInline(vg, inline_,
                      content_cx, ry + rh * 0.5f,
                      NVG_ALIGN_CENTER, NVG_ALIGN_MIDDLE,
                      size,
                      icon_gain,
                      icon_pad_t,
                      face.c_str(),
                      default_text,
                      default_icon);
    } else {
        Inline::Run text_run;
        text_run.type = Inline::Run::Type::Text;
        text_run.utf8 = label_;
        const std::vector<Inline::Run> runs = { std::move(text_run) };
        renderInline(vg, runs,
                      content_cx, ry + rh * 0.5f,
                      NVG_ALIGN_CENTER, NVG_ALIGN_MIDDLE,
                      size,
                      icon_gain,
                      icon_pad_t,
                      face.c_str(),
                      default_text,
                      default_icon);
    }

    // ── Dropdown chevron indicator ──
    if (dropdownMenu_) {
        float chev_s = size * icon_gain * 0.7f;
        float chev_x = rx + rw - size * icon_gain * 1.0f;
        float chev_y = ry + (rh - chev_s) * 0.5f;
        NVGcolor chev_col = nvgRGBA(
            theme.colors.on_surface.r,
            theme.colors.on_surface.g,
            theme.colors.on_surface.b,
            theme.colors.on_surface.a / 2);
        Icons::renderIcon(vg, Icons::chevron_down, chev_x, chev_y, chev_s, chev_col);
    }
}
void Button::updateHover(float mx, float my) noexcept {
    hovered_ = containsPoint(mx, my);
}
void Button::tick(float dt, const Theme& theme) noexcept {
    hoverAnim_.setTarget(hovered_ ? 1.0f : 0.0f);
    pressAnim_.setTarget(pressed_ ? 1.0f : 0.0f);
    hoverAnim_.tick(dt, theme.metrics.animation.hover_tau);
    pressAnim_.tick(dt, theme.metrics.animation.press_tau);
}
EventResult Button::handleEvent(const Event& e, const Window& w) {
    if (!enabled_) return EventResult::Ignored;
    float mx{}, my{};
    w.cursorPosition(mx, my);
    const bool inside = containsPoint(mx, my);
    switch (kind_of(e)) {
    case EventKind::MouseMove: {
        return inside ? EventResult::Consumed : EventResult::Ignored;
    }
    case EventKind::MouseDown: {
        if (inside) {
            pressed_ = true;
            return EventResult::Consumed;
        }
        return EventResult::Ignored;
    }
    case EventKind::MouseUp: {
        const bool was_pressed = pressed_;
        pressed_ = false;
        if (was_pressed && inside) {
            if (dropdownMenu_) {
                // Show dropdown popup menu below the button
                auto popup = Menu::makePopup(dropdownMenu_->items(),
                                             const_cast<Window&>(w));
                const Theme& theme = w.theme();
                const float natural_w = popup->naturalWidth(theme, w);
                const float natural_h = popup->naturalHeight(theme, w);
                float p_x = bounds_.x + bounds_.w - natural_w;
                float p_y = bounds_.y + bounds_.h;
                const auto [win_w, win_h] = w.designSize();
                // If it overflows the window bottom, wrap in a scrollable container
                float max_h = natural_h;
                if (p_y + natural_h > static_cast<float>(win_h)) {
                    max_h = (std::max)(0.0f, static_cast<float>(win_h) - p_y);
                }
                // Clamp horizontal
                if (p_x < 0.0f) p_x = 0.0f;
                if (p_x + natural_w > static_cast<float>(win_w))
                    p_x = static_cast<float>(win_w) - natural_w;

                std::unique_ptr<Widget> widget;
                if (max_h < natural_h) {
                    auto scroll = std::make_unique<Container>(
                        Rect(p_x, p_y, natural_w, max_h));
                    scroll->setScrollable(true);
                    scroll->setAutoSize(AutoSize::Disabled);
                    auto vl = std::make_unique<VerticalLayout>();
                    scroll->setLayout(std::move(vl));
                    popup->setBounds(Rect(0, 0, natural_w, natural_h));
                    scroll->addChild(std::move(popup));
                    widget = std::move(scroll);
                } else {
                    popup->setBounds(Rect(p_x, p_y, natural_w, natural_h));
                    widget = std::move(popup);
                }
                const_cast<Window&>(w).showPopup(std::move(widget));
            } else {
                if (on_click_) on_click_();
            }
            triggerEvent("onClick");            
            return EventResult::Consumed;
        }
        return inside ? EventResult::Consumed : EventResult::Ignored;
    }
    default:
        return EventResult::Ignored;
    }
}
}  

