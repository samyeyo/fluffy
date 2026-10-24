// src/menu.cpp

// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
// Menu — horizontal or vertical menu widget with animated hover highlights.
//
// MIT License - Copyright (c) 2026 - Samir Tine
// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

#include <fluffy/menu.hpp>

#include <algorithm>

#include <nanovg.h>

#include <fluffy/icons.hpp>
#include <fluffy/inline.hpp>
#include <fluffy/window.hpp>
#include <fluffy/yaml_fwd.hpp>

namespace fluffy {

// ––––––––[ Menu ] constructs with bounds and orientation.
Menu::Menu(Rect bounds, Orientation orient)
    : Widget(bounds),
      orientation_(orient)
{}

// ––––––––[ itemHeight ] uniform row height based on text size and padding.
float Menu::itemHeight(const Theme& theme) const noexcept {
    const float size = textSize_ > 0.0f ? textSize_ : theme.metrics.text_size;
    return size + theme.metrics.button_padding_y * 2.0f;
}

// ––––––––[ itemAt ] returns index of item at (mx, my) or -1 if none.
int Menu::itemAt(float mx, float my, const Theme& theme, const Window& w) const noexcept {
    if (!containsPoint(mx, my)) return -1;
    if (items_.empty()) return -1;
    const float rx = bounds_.x;
    const float ry = bounds_.y;
    const float size = textSize_ > 0.0f ? textSize_ : theme.metrics.text_size;
    const float padX = theme.metrics.button_padding_x;

    if (orientation_ == Orientation::Vertical) {
        const float ih = itemHeight(theme);
        const int idx = static_cast<int>((my - ry) / ih);
        if (idx < 0 || idx >= static_cast<int>(items_.size())) return -1;
        return idx;
    }

    // Horizontal: measure each item's actual rendered width via nvgTextBounds,
    // matching draw() exactly so hit-test regions align with the visual hover rect.
    NVGcontext* vg = w.nvgContext();
    if (!vg) {
        // Fallback if no context yet
        float cx = rx + padX;
        const float icon_s = size * theme.metrics.animation.icon_size_gain;
        for (std::size_t i = 0; i < items_.size(); ++i) {
            const auto& item = items_[i];
            if (item.separator) { cx += 12.0f; continue; }
            float iw = padX;
            if (item.icon.svg_data) iw += icon_s + 4.0f;
            iw += item.label.length() * size * 0.55f;
            if (mx >= cx && mx < cx + iw) return static_cast<int>(i);
            cx += iw + 2.0f;
        }
        return -1;
    }

    const std::string spec = font_.empty() ? theme.font_family : font_;
    std::string face;
    const_cast<Window&>(w).prepareFont(vg, spec, face);
    nvgFontSize(vg, size);
    nvgFontFace(vg, face.c_str());
    const float icon_s = size * theme.metrics.animation.icon_size_gain;
    float cx = rx + padX;
    for (std::size_t i = 0; i < items_.size(); ++i) {
        const auto& item = items_[i];
        if (item.separator) { cx += 12.0f; continue; }
        float iw = 0.0f;
        if (item.icon.svg_data) iw += icon_s + 4.0f;
        if (!item.label.empty()) {
            iw += nvgTextBounds(vg, 0, 0, item.label.c_str(), nullptr, nullptr);
        }
        iw += padX;
        if (mx >= cx && mx < cx + iw) return static_cast<int>(i);
        cx += iw + 2.0f;
    }
    return -1;
}

// ––––––––[ setProperty ] base widget properties only (items are added programmatically).
void Menu::setProperty(std::string_view name, const YamlNode& value) {
    Widget::setProperty(name, value);
}

// ––––––––[ naturalWidth ] max item width for vertical; sum of item widths for horizontal.
float Menu::naturalWidth(const Theme& theme, const Window& w) const {
    if (items_.empty()) return 0.0f;
    const float size = textSize_ > 0.0f ? textSize_ : theme.metrics.text_size;
    const float padX = theme.metrics.button_padding_x;

    if (orientation_ == Orientation::Horizontal) {
        float total = 0.0f;
        for (const auto& item : items_) {
            if (item.separator) { total += 12.0f; continue; }
            float iw = padX;
            if (!item.inlineLabel.empty()) {
                // Measure inline runs
                NVGcontext* vg2 = w.nvgContext();
                if (vg2) {
                    const std::string spec = font_.empty() ? theme.font_family : font_;
                    std::string face2;
                    const_cast<Window&>(w).prepareFont(vg2, spec, face2);
                    nvgFontSize(vg2, size);
                    nvgFontFace(vg2, face2.c_str());
                    for (auto& r : item.inlineLabel.runs()) {
                        if (r.type == Inline::Run::Type::SvgIcon) {
                            float rs = r.size > 0.0f ? r.size : size;
                            iw += rs * theme.metrics.animation.icon_size_gain + 4.0f;
                        } else {
                            float rs = r.size > 0.0f ? r.size : size;
                            nvgFontSize(vg2, rs);
                            iw += nvgTextBounds(vg2, 0, 0, r.utf8.c_str(), nullptr, nullptr);
                            nvgFontSize(vg2, size);
                        }
                    }
                } else {
                    for (auto& r : item.inlineLabel.runs()) if (r.type == Inline::Run::Type::Text) iw += r.utf8.length() * size * 0.55f;
                }
            } else {
                if (item.icon.svg_data) iw += size * theme.metrics.animation.icon_size_gain + 4.0f;
                if (!item.label.empty()) iw += item.label.length() * size * 0.55f;
            }
            total += iw + 2.0f;
        }
        return total + padX;
    }

    // Vertical: max item width among all items
    const float icon_gain = theme.metrics.animation.icon_size_gain;
    NVGcontext* vg = w.nvgContext();
    float max_w = 0.0f;
    for (const auto& item : items_) {
        if (item.separator) continue;
        float iw = 0.0f;
        if (!item.inlineLabel.empty()) {
            if (vg) {
                const std::string spec = font_.empty() ? theme.font_family : font_;
                std::string face2;
                const_cast<Window&>(w).prepareFont(vg, spec, face2);
                nvgFontSize(vg, size);
                nvgFontFace(vg, face2.c_str());
                const float icon_pad = theme.metrics.animation.icon_padding;
                for (auto& r : item.inlineLabel.runs()) {
                    if (r.type == Inline::Run::Type::SvgIcon) {
                        float rs = r.size > 0.0f ? r.size : size;
                        iw += rs * icon_gain + 2.0f * rs * icon_gain * icon_pad + 2.0f;
                    } else {
                        float rs = r.size > 0.0f ? r.size : size;
                        nvgFontSize(vg, rs);
                        iw += nvgTextBounds(vg, 0, 0, r.utf8.c_str(), nullptr, nullptr);
                        nvgFontSize(vg, size);
                    }
                }
            } else {
                for (auto& r : item.inlineLabel.runs()) if (r.type == Inline::Run::Type::Text) iw += r.utf8.length() * size * 0.55f;
            }
        } else {
            if (item.icon.svg_data) iw += size * icon_gain + 6.0f;
            if (vg && !item.label.empty()) {
                const std::string spec = font_.empty() ? theme.font_family : font_;
                std::string face;
                const_cast<Window&>(w).prepareFont(vg, spec, face);
                nvgFontSize(vg, size);
                nvgFontFace(vg, face.c_str());
                iw += nvgTextBounds(vg, 0, 0, item.label.c_str(), nullptr, nullptr);
            } else {
                iw += item.label.length() * size * 0.55f;
            }
        }
        // Extra space for submenu chevron indicator
        if (item.submenu) iw += size * icon_gain * 0.7f;
        max_w = (std::max)(max_w, iw);
    }
    return max_w + padX * 2.0f;
}

// ––––––––[ naturalHeight ] rows * item height for vertical; one row for horizontal.
float Menu::naturalHeight(const Theme& theme, const Window&) const {
    if (items_.empty()) return 0.0f;
    const float ih = itemHeight(theme);
    if (orientation_ == Orientation::Vertical) {
        return ih * static_cast<float>(items_.size());
    }
    return ih;
}

// ––––––––[ draw ] renders the menu with Fluent UI styling, items, hover highlights, and focus ring.
void Menu::draw(NVGcontext* vg, const Theme& theme, const Window& w) {
    if (!visible_ || items_.empty()) return;
    const float rx = bounds_.x, ry = bounds_.y;
    const float rw = bounds_.w, rh = bounds_.h;
    const float size = textSize_ > 0.0f ? textSize_ : theme.metrics.text_size;
    const float ih   = itemHeight(theme);
    const float padX = theme.metrics.button_padding_x;
    const float cr   = theme.metrics.corner_radius;
    const float t_h  = hoverAnim_.value();
    const Color c_accent  = accentColor(theme);

    // ── Background: shadow + surface fill for vertical dropdown only; horizontal menubar is transparent ──
    if (orientation_ == Orientation::Vertical) {
        const Shadow& sh = theme.metrics.shadow;
        if (sh.blur > 0.0f && sh.color.a > 0) {
            NVGcolor sh_inner = nvgRGBA(sh.color.r, sh.color.g, sh.color.b, sh.color.a);
            NVGcolor sh_outer = nvgRGBA(sh.color.r, sh.color.g, sh.color.b, 0);
            NVGpaint sh_paint = nvgBoxGradient(vg,
                rx + sh.offset_x, ry + sh.offset_y,
                rw, rh, cr, sh.blur, sh_inner, sh_outer);
            nvgBeginPath(vg);
            nvgRoundedRect(vg, rx - sh.blur, ry - sh.blur, rw + sh.blur * 2.0f, rh + sh.blur * 2.0f, cr);
            nvgFillPaint(vg, sh_paint);
            nvgFill(vg);
        }

        nvgBeginPath(vg);
        nvgRoundedRect(vg, rx, ry, rw, rh, cr);
        nvgFillColor(vg, nvgRGBA(theme.colors.surface.r,
                                  theme.colors.surface.g,
                                  theme.colors.surface.b,
                                  theme.colors.surface.a));
        nvgFill(vg);
        nvgStrokeWidth(vg, theme.metrics.border_width);
        nvgStrokeColor(vg, nvgRGBA(theme.colors.border.r,
                                    theme.colors.border.g,
                                    theme.colors.border.b,
                                    theme.colors.border.a));
        nvgStroke(vg);
    }

    // ── Font setup ──
    const std::string spec = font_.empty() ? theme.font_family : font_;
    std::string face;
    w.prepareFont(vg, spec, face);
    nvgFontSize(vg, size);
    nvgFontFace(vg, face.c_str());
    const float icon_gain = theme.metrics.animation.icon_size_gain;
    const float icon_s    = size * icon_gain;

    if (orientation_ == Orientation::Vertical) {
        // ── Vertical: stacked rows (Fluent UI dropdown style) ──
        for (std::size_t i = 0; i < items_.size(); ++i) {
            const auto& item = items_[i];
            const float iy = ry + static_cast<float>(i) * ih;

            // Separator line — thin, centered vertically in the row, with horizontal margins
            if (item.separator) {
                const float sy = iy + ih * 0.5f;
                nvgBeginPath(vg);
                nvgMoveTo(vg, rx + 12.0f, sy);
                nvgLineTo(vg, rx + rw - 12.0f, sy);
                nvgStrokeWidth(vg, 1.0f);
                NVGcolor sep_col = nvgRGBA(theme.colors.border.r,
                                            theme.colors.border.g,
                                            theme.colors.border.b,
                                            theme.colors.border.a / 2);
                nvgStrokeColor(vg, sep_col);
                nvgStroke(vg);
                continue;
            }

            // Hover highlight — full width with 2px margin, subtle accent color (Fluent UI style)
            if (static_cast<int>(i) == hovered_index_ && item.enabled) {
                const float hov_alpha = 0.08f + t_h * 0.06f;
                nvgBeginPath(vg);
                nvgRoundedRect(vg, rx + 2.0f, iy + 1.0f, rw - 4.0f, ih - 2.0f, 2.0f);
                nvgFillColor(vg, nvgRGBA(c_accent.r, c_accent.g, c_accent.b,
                                          static_cast<std::uint8_t>(c_accent.a * hov_alpha)));
                nvgFill(vg);
            }

            // Icon / Label — inline if present, else separate icon+label
            float tx = rx + padX;
            const float ly = iy + ih * 0.5f;
            if (!item.inlineLabel.empty()) {
                // Clip inline to row
                nvgSave(vg);
                nvgIntersectScissor(vg, rx + 2.0f, iy + 1.0f, rw - 4.0f, ih - 2.0f);
                Color def = item.enabled ? theme.colors.on_surface : Color{theme.colors.on_surface.r, theme.colors.on_surface.g, theme.colors.on_surface.b, static_cast<uint8_t>(theme.colors.on_surface.a / 2)};
                renderInline(vg, item.inlineLabel.runs(), tx, ly,
                             NVG_ALIGN_LEFT, NVG_ALIGN_MIDDLE, size,
                             theme.metrics.animation.icon_size_gain, theme.metrics.animation.icon_padding,
                             face.c_str(), def, def);
                nvgRestore(vg);
            } else {
                if (item.icon.svg_data) {
                    const float icon_y = iy + (ih - icon_s) * 0.5f;
                    NVGcolor icon_col = nvgRGBA(
                        c_accent.r, c_accent.g, c_accent.b,
                        item.enabled ? c_accent.a
                                     : static_cast<std::uint8_t>(c_accent.a * 0.4f));
                    Icons::renderIcon(vg, item.icon, tx, icon_y, icon_s, icon_col);
                    tx += icon_s + 6.0f;
                }
                // Label
                nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
                if (!item.label.empty()) {
                    nvgFillColor(vg, nvgRGBA(theme.colors.on_surface.r,
                                              theme.colors.on_surface.g,
                                              theme.colors.on_surface.b,
                                              item.enabled ? theme.colors.on_surface.a
                                                           : theme.colors.on_surface.a / 2));
                    nvgText(vg, tx, ly, item.label.c_str(), nullptr);
                }
            }

            // Submenu chevron_right indicator — visible, same size as item icon
            if (item.submenu) {
                const float ch_s = size * icon_gain * 0.65f;
                float ch_x = rx + rw - padX - ch_s;
                float ch_y = iy + (ih - ch_s) * 0.5f;
                NVGcolor ch_col = nvgRGBA(
                    theme.colors.on_surface.r,
                    theme.colors.on_surface.g,
                    theme.colors.on_surface.b,
                    theme.colors.on_surface.a);
                Icons::renderIcon(vg, Icons::chevron_right, ch_x, ch_y, ch_s, ch_col);
            }

        }
    } else {
        // ── Horizontal: items in a row (Fluent UI menubar style, no background) ──
        float cx = rx + padX;
        for (std::size_t i = 0; i < items_.size(); ++i) {
            const auto& item = items_[i];

            // Vertical separator
            if (item.separator) {
                const float sx = cx + 4.0f;
                nvgBeginPath(vg);
                nvgMoveTo(vg, sx, ry + 4.0f);
                nvgLineTo(vg, sx, ry + rh - 4.0f);
                nvgStrokeWidth(vg, 1.0f);
                NVGcolor sep_col = nvgRGBA(theme.colors.border.r,
                                            theme.colors.border.g,
                                            theme.colors.border.b,
                                            theme.colors.border.a / 2);
                nvgStrokeColor(vg, sep_col);
                nvgStroke(vg);
                cx += 12.0f;
                continue;
            }

            // Measure item width for hover rect
            float iw = 0.0f;
            if (item.icon.svg_data) iw += icon_s + 4.0f;
            if (!item.label.empty()) {
                iw += nvgTextBounds(vg, 0, 0, item.label.c_str(), nullptr, nullptr);
            }
            iw += padX;

            // Hover highlight — subtle accent, 2px margin from top/bottom
            if (static_cast<int>(i) == hovered_index_ && item.enabled) {
                const float hov_alpha = 0.08f + t_h * 0.06f;
                nvgBeginPath(vg);
                nvgRoundedRect(vg, cx + 1.0f, ry + 2.0f, iw - 2.0f, rh - 4.0f, 2.0f);
                nvgFillColor(vg, nvgRGBA(c_accent.r, c_accent.g, c_accent.b,
                                          static_cast<std::uint8_t>(c_accent.a * hov_alpha)));
                nvgFill(vg);
            }

            // Icon / Label — inline if present
            float lx = cx + 4.0f;
            if (!item.inlineLabel.empty()) {
                nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
                const float ly = ry + rh * 0.5f;
                Color def = item.enabled ? theme.colors.on_surface : Color{theme.colors.on_surface.r, theme.colors.on_surface.g, theme.colors.on_surface.b, static_cast<uint8_t>(theme.colors.on_surface.a / 2)};
                // Scissor inline to item width
                float iw2 = 0.0f;
                for (auto& r : item.inlineLabel.runs()) {
                    if (r.type == Inline::Run::Type::SvgIcon) iw2 += size * theme.metrics.animation.icon_size_gain + 4.0f;
                    else iw2 += r.utf8.length() * size * 0.55f;
                }
                renderInline(vg, item.inlineLabel.runs(), lx, ly,
                             NVG_ALIGN_LEFT, NVG_ALIGN_MIDDLE, size,
                             theme.metrics.animation.icon_size_gain, theme.metrics.animation.icon_padding,
                             face.c_str(), def, def);
                lx += iw2 + 4.0f;
            } else {
                if (item.icon.svg_data) {
                    const float icon_y = ry + (rh - icon_s) * 0.5f;
                    NVGcolor icon_col = nvgRGBA(
                        c_accent.r, c_accent.g, c_accent.b,
                        item.enabled ? c_accent.a
                                     : static_cast<std::uint8_t>(c_accent.a * 0.4f));
                    Icons::renderIcon(vg, item.icon, lx, icon_y, icon_s, icon_col);
                    lx += icon_s + 4.0f;
                }
                // Label
                if (!item.label.empty()) {
                    nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
                    const float ly = ry + rh * 0.5f;
                    nvgFillColor(vg, nvgRGBA(theme.colors.on_surface.r,
                                              theme.colors.on_surface.g,
                                              theme.colors.on_surface.b,
                                              item.enabled ? theme.colors.on_surface.a
                                                           : theme.colors.on_surface.a / 2));
                    nvgText(vg, lx, ly, item.label.c_str(), nullptr);
                }
            }

            cx += iw + 2.0f;
        }
    }

    // Focus ring
    if (focused_) {
        drawFocusRing(vg, bounds_, theme);
    }
}

// ––––––––[ makePopup ] deep-copies items into a standalone vertical popup menu, wrapping each
// on_click to close the owning window's popup first. Recursively handles submenus.
std::unique_ptr<Menu> Menu::makePopup(const std::vector<Item>& source, Window& w) {
    auto popup = std::make_unique<Menu>(Rect{}, Orientation::Vertical);
    for (const auto& src : source) {
        Item copy;
        copy.icon     = src.icon;
        copy.label    = src.label;
        copy.inlineLabel = src.inlineLabel;
        copy.shortcut = src.shortcut;
        copy.enabled  = src.enabled;
        copy.separator = src.separator;
        copy.on_click = [&w, cb = src.on_click]() {
            const_cast<Window&>(w).closePopupDeferred();
            if (cb) cb();
        };
        if (src.submenu) {
            copy.submenu = makePopup(src.submenu->items(), w);
        }
        popup->addItem(std::move(copy));
    }
    return popup;
}

// ––––––––[ updateHover ] sets global hover state for animation.
void Menu::updateHover(float mx, float my) noexcept {
    hoverAnim_.setTarget(containsPoint(mx, my) ? 1.0f : 0.0f);
}

// ––––––––[ tick ] drives hover animation.
void Menu::tick(float dt, const Theme& theme) noexcept {
    (void)theme;
    hoverAnim_.tick(dt, 0.08f);
}

// ––––––––[ handleEvent ] mouse input: track hovered item, fire on_click on release.
EventResult Menu::handleEvent(const Event& e, const Window& w) {
    if (!enabled_) return EventResult::Ignored;
    float mx{}, my{};
    w.cursorPosition(mx, my);

    switch (kind_of(e)) {
    case EventKind::MouseMove: {
        const bool inside = containsPoint(mx, my);
        const int idx = itemAt(mx, my, w.theme(), w);
        if (idx != hovered_index_) {
            hovered_index_ = idx;
        }
        return inside ? EventResult::Consumed : EventResult::Ignored;
    }
    case EventKind::MouseDown: {
        if (containsPoint(mx, my)) {
            return EventResult::Consumed;
        }
        return EventResult::Ignored;
    }
    case EventKind::MouseUp: {
        const bool inside = containsPoint(mx, my);
        if (inside) {
            const int idx = itemAt(mx, my, w.theme(), w);
            if (idx >= 0 && idx < static_cast<int>(items_.size())) {
                auto& item = items_[static_cast<std::size_t>(idx)];
                if (item.enabled && !item.separator) {
                    // Open submenu popup for horizontal or vertical menu items
                    if (item.submenu) {
                        // Calculate the x-position of the clicked item
                        const Theme& theme = w.theme();
                        const float padX = theme.metrics.button_padding_x;
                        const float size = textSize_ > 0.0f ? textSize_ : theme.metrics.text_size;
                        const float icon_gain = theme.metrics.animation.icon_size_gain;
                        const float icon_s = size * icon_gain;
                        float cx = bounds_.x + padX;
                        for (int si = 0; si < idx; ++si) {
                            const auto& si_item = items_[static_cast<std::size_t>(si)];
                            if (si_item.separator) { cx += 12.0f; continue; }
                            float siw = 0.0f;
                            if (si_item.icon.svg_data) siw += icon_s + 4.0f;
                            if (!si_item.label.empty()) {
                                NVGcontext* vg = const_cast<Window&>(w).nvgContext();
                                if (vg) {
                                    float tb[4];
                                    nvgTextBounds(vg, 0, 0, si_item.label.c_str(), nullptr, tb);
                                    siw += tb[2] - tb[0];
                                } else {
                                    siw += si_item.label.length() * size * 0.55f;
                                }
                            }
                            siw += padX;
                            cx += siw + 2.0f;
                        }

                        // Build popup menu from submenu items
                        auto popup = Menu::makePopup(item.submenu->items(),
                                                     const_cast<Window&>(w));

                        // Position below or to the right of the clicked item
                        float p_x, p_y;
                        if (orientation_ == Orientation::Horizontal) {
                            p_x = cx;
                            p_y = bounds_.y + bounds_.h;
                        } else {
                            // Vertical: submenu appears to the right, aligned with top of item
                            p_x = bounds_.x + bounds_.w;
                            p_y = bounds_.y + static_cast<float>(idx) * itemHeight(theme);
                        }
                        const float natural_w = popup->naturalWidth(theme, w) * 1.5f;
                        const float natural_h = popup->naturalHeight(theme, w);
                        popup->setBounds(Rect(p_x, p_y, natural_w, natural_h));

                        // For horizontal menubar: close previous dropdown before showing new one
                        // For vertical (cascading submenus): keep the parent menu visible
                        if (orientation_ == Orientation::Horizontal) {
                            const_cast<Window&>(w).closePopup();
                        }
                        const_cast<Window&>(w).showPopup(std::move(popup));
                    } else if (item.on_click) {
                        item.on_click();
                    }
                }
            }
            return EventResult::Consumed;
        }
        return EventResult::Ignored;
    }
    default:
        return EventResult::Ignored;
    }
}

} // namespace fluffy
