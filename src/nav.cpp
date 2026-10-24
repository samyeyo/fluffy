// src/nav.cpp

// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
// Nav — hierarchical navigation control with expandable groups and items.
//
// MIT License - Copyright (c) 2026 - Samir Tine
// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

#include <fluffy/nav.hpp>

#include <algorithm>
#include <cmath>
#if defined(FLUFFY_EMSCRIPTEN) || defined(__EMSCRIPTEN__)
#include <emscripten.h>
#endif

#include <nanovg.h>

#include <GLFW/glfw3.h>

#include <fluffy/icons.hpp>
#include <fluffy/inline.hpp>
#include <fluffy/window.hpp>
#include <fluffy/yaml_fwd.hpp>

namespace fluffy {

// ––––––––[ Nav ] constructs with bounds rectangle.
Nav::Nav(Rect bounds)
    : Widget(bounds)
{}

// ––––––––[ addGroup ] appends an expandable category.
void Nav::addGroup(NavGroup group) {
    groups_.push_back(std::move(group));
}

// ––––––––[ addItem ] appends an ungrouped item at the root level.
void Nav::addItem(NavItem item) {
    rootItems_.push_back(std::move(item));
}

// ––––––––[ expandGroup ] expands a group by id (triggers animation).
void Nav::expandGroup(const std::string& id) {
    for (auto& g : groups_) {
        if (g.id == id) {
            g.expanded = true;
            return;
        }
    }
}

// ––––––––[ collapseGroup ] collapses a group by id (triggers animation).
void Nav::collapseGroup(const std::string& id) {
    for (auto& g : groups_) {
        if (g.id == id) {
            g.expanded = false;
            return;
        }
    }
}

// ––––––––[ expandAll ] expands all groups.
void Nav::expandAll() {
    for (auto& g : groups_) {
        g.expanded = true;
    }
}

// ––––––––[ collapseAll ] collapses all groups.
void Nav::collapseAll() {
    for (auto& g : groups_) {
        g.expanded = false;
    }
}

// ––––––––[ rowHeight ] uniform row height based on text size and padding.
float Nav::rowHeight(const Theme& theme) const noexcept {
    const float size = textSize_ > 0.0f ? textSize_ : theme.metrics.text_size;
    return size + theme.metrics.button_padding_y * 2.0f;
}

// ––––––––[ indentPx ] pixel width for one indent level.
float Nav::indentPx(const Theme& theme) const noexcept {
    const float size = textSize_ > 0.0f ? textSize_ : theme.metrics.text_size;
    return size * 1.2f;
}

// ––––––––[ rebuildLines ] flattens groups + root items into a visible row array using animated heights.
void Nav::rebuildLines(const Theme& theme) const {
    lines_.clear();
    const float rh = rowHeight(theme);
    float cy = bounds_.y - scrollY_;

    for (std::size_t gi = 0; gi < groups_.size(); ++gi) {
        const auto& g = groups_[gi];
        const float anim_t = g.expandAnim.value();

        // Group header
        RenderedLine header;
        header.id             = g.id;
        header.label          = g.label;
        header.icon           = g.icon;
        header.y              = cy;
        header.h              = rh;
        header.indent         = 0;
        header.isItem         = false;
        header.enabled        = g.enabled;
        header.groupIdx       = gi;
        header.itemIdx        = 0;
        header.isGroupHeader  = true;
        header.groupExpanded  = anim_t > 0.5f;
        lines_.push_back(std::move(header));
        cy += rh;

        // Items — height scales with animation value for smooth expand/collapse
        if (anim_t > 0.001f) {
            for (std::size_t ii = 0; ii < g.items.size(); ++ii) {
                const auto& item = g.items[ii];
                const float item_h = rh * anim_t;
                if (item_h < 0.5f) continue;
                RenderedLine line;
                line.id             = item.id;
                line.label          = item.label;
                line.icon           = item.icon;
                line.y              = cy;
                line.h              = item_h;
                line.indent         = 1;
                line.isItem         = true;
                line.enabled        = item.enabled;
                line.groupIdx       = gi;
                line.itemIdx        = ii;
                line.isGroupHeader  = false;
                line.on_click       = item.on_click;
                lines_.push_back(std::move(line));
                cy += item_h;
            }
        }
    }

    // Root items — always visible at full height
    for (const auto& item : rootItems_) {
        RenderedLine line;
        line.id       = item.id;
        line.label    = item.label;
        line.icon     = item.icon;
        line.y        = cy;
        line.h        = rh;
        line.indent   = 0;
        line.isItem   = true;
        line.enabled  = item.enabled;
        line.isGroupHeader = false;
        line.on_click = item.on_click;
        lines_.push_back(std::move(line));
        cy += rh;
    }
}

// ––––––––[ naturalWidth ] returns the maximum content width + padding.
float Nav::naturalWidth(const Theme& theme, const Window& w) const {
    const float size = textSize_ > 0.0f ? textSize_ : theme.metrics.text_size;
    const float padX = theme.metrics.button_padding_x;
    const float ip   = indentPx(theme);
    const float icon_gain = theme.metrics.animation.icon_size_gain;
    const float icon_s    = size * icon_gain;
    const float arrow_space = size * icon_gain * 0.55f + 6.0f;
    NVGcontext* vg = w.nvgContext();
    float max_w = padX * 2.0f;

    auto measure_label = [&](const std::string& label, int indent) -> float {
        float lw = padX + arrow_space + ip * static_cast<float>(indent);
        if (vg && !label.empty()) {
            const std::string spec = font_.empty() ? theme.font_family : font_;
            std::string face;
            const_cast<Window&>(w).prepareFont(vg, spec, face);
            nvgFontSize(vg, size);
            nvgFontFace(vg, face.c_str());
            lw += nvgTextBounds(vg, 0, 0, label.c_str(), nullptr, nullptr);
        } else {
            lw += label.length() * size * 0.55f;
        }
        return lw;
    };

    for (const auto& g : groups_) {
        float gw = padX + arrow_space;
        if (g.icon.svg_data) gw += icon_s + 4.0f;
        gw += g.label.length() * size * 0.55f;
        max_w = (std::max)(max_w, gw + padX);

        for (const auto& item : g.items) {
            float iw = measure_label(item.label, 1);
            if (item.icon.svg_data) iw += icon_s + 4.0f;
            max_w = (std::max)(max_w, iw);
        }
    }

    for (const auto& item : rootItems_) {
        float iw = measure_label(item.label, 0);
        if (item.icon.svg_data) iw += icon_s + 4.0f;
        max_w = (std::max)(max_w, iw);
    }

    return max_w;
}

// ––––––––[ naturalHeight ] total height of all visible rows.
float Nav::naturalHeight(const Theme& theme, const Window&) const {
    rebuildLines(theme);
    if (lines_.empty()) return 0.0f;
    const float first_y = lines_.front().y;
    const float last_b = lines_.back().y + lines_.back().h;
    return last_b - first_y;
}

// ––––––––[ computeMinWidth ] measures the widest first-level item (group headers + root items).
void Nav::computeMinWidth(const Theme& theme, const Window& w) {
    const float size = textSize_ > 0.0f ? textSize_ : theme.metrics.text_size;
    const float padX = theme.metrics.button_padding_x;
    const float icon_gain = theme.metrics.animation.icon_size_gain;
    const float icon_s    = size * icon_gain;
    const float arrow_space = size * icon_gain * 0.55f + 6.0f;
    NVGcontext* vg = w.nvgContext();
    float max_w = padX * 2.0f;

    auto measure = [&](const std::string& label) -> float {
        if (vg && !label.empty()) {
            const std::string spec = font_.empty() ? theme.font_family : font_;
            std::string face;
            const_cast<Window&>(w).prepareFont(vg, spec, face);
            nvgFontSize(vg, size);
            nvgFontFace(vg, face.c_str());
            return nvgTextBounds(vg, 0, 0, label.c_str(), nullptr, nullptr);
        }
        return label.length() * size * 0.55f;
    };

    for (const auto& g : groups_) {
        float gw = padX + arrow_space;
        if (g.icon.svg_data) gw += icon_s + 4.0f;
        gw += measure(g.label);
        max_w = (std::max)(max_w, gw + padX);
    }

    for (const auto& item : rootItems_) {
        float iw = padX + arrow_space;
        if (item.icon.svg_data) iw += icon_s + 4.0f;
        iw += measure(item.label);
        max_w = (std::max)(max_w, iw + padX);
    }

    minWidth_ = max_w;
}

// ––––––––[ setSelectedItem ] selects an item by id, clears if empty.
void Nav::setSelectedItem(const std::string& id) {
    if (selectedId_ == id) return;
    selectedId_ = id;
    if (on_select_) on_select_(id);
}

// ––––––––[ setProperty ] supports \"selected\", \"accent\", plus base props.
void Nav::setProperty(std::string_view name, const YamlNode& value) {
    if (name == "selected") { setSelectedItem(value.asString()); return; }
    Widget::setProperty(name, value);
}

// ––––––––[ draw ] renders the Nav with Fluent UI styling.
void Nav::draw(NVGcontext* vg, const Theme& theme, const Window& w) {
    if (!visible_) return;

    // ── Clamp scrollY_ before rebuilding lines so the first frame is always valid ──
    {
        // Estimate total content height by measuring all rows at current scrollY_
        const float rh = rowHeight(theme);
        float estimatedH = 0.0f;
        for (const auto& g : groups_) {
            estimatedH += rh;  // header
            if (g.expandAnim.value() > 0.001f)
                estimatedH += rh * g.expandAnim.value() * static_cast<float>(g.items.size());
        }
        estimatedH += rh * static_cast<float>(rootItems_.size());
        const float maxScroll = (std::max)(0.0f, estimatedH - bounds_.h);
        scrollY_ = (std::min)(scrollY_, maxScroll);
        if (estimatedH <= bounds_.h + 1.0f) scrollY_ = 0.0f;
    }

    rebuildLines(theme);

    const float rx = bounds_.x, ry = bounds_.y;
    const float rw = bounds_.w;
    const float size = textSize_ > 0.0f ? textSize_ : theme.metrics.text_size;
    const float padX = theme.metrics.button_padding_x;
    const float ip = indentPx(theme);
    const float rh = rowHeight(theme);
    const float icon_gain = theme.metrics.animation.icon_size_gain;
    const float icon_s = size * icon_gain;
    const float ch_s = size * icon_gain * 0.55f;    // chevron size
    const float arrow_space = ch_s + 6.0f;            // chevron + gap
    const Color c_accent = accentColor(theme);
    const float t_h = hoverAnim_.value();
    const float sbAlpha = scrollAnim_.value();

    // ── Clip content to widget bounds — NanoVG's scissor works in logical coordinates
    //     because the current transform (set by nvgBeginFrame) already handles pixel ratio ──
    nvgSave(vg);
    nvgScissor(vg, rx, ry, rw, bounds_.h);

    // ── Background ──
    nvgBeginPath(vg);
    nvgRoundedRect(vg, rx, ry, rw, bounds_.h, theme.metrics.corner_radius);
    nvgFillColor(vg, nvgRGBA(theme.colors.surface.r,
                              theme.colors.surface.g,
                              theme.colors.surface.b,
                              theme.colors.surface.a));
    nvgFill(vg);

    // ── Font setup ──
    const std::string spec = font_.empty() ? theme.font_family : font_;
    std::string face;
    w.prepareFont(vg, spec, face);
    // Resolve bold variant if available — real bold replaces faux double-draw.
    const std::string bold_face = face + "-bold";
    const bool has_bold = nvgFindFont(vg, bold_face.c_str()) != -1;
    nvgFontSize(vg, size);
    nvgFontFace(vg, face.c_str());

    for (std::size_t i = 0; i < lines_.size(); ++i) {
        const auto& line = lines_[i];
        const float ly = line.y;
        const float lh = line.h;

        // ── Content fade for animating items — fades out as item shrinks ──
        const float content_fade = line.isItem && !line.isGroupHeader
            ? (std::min)(1.0f, lh / (rh * 0.5f))
            : 1.0f;
        if (content_fade < 0.01f) continue;

        // ── Hover highlight ──
        if (static_cast<int>(i) == hoveredIdx_ && line.enabled) {
            const float hov_alpha = (0.06f + t_h * 0.06f) * content_fade;
            nvgBeginPath(vg);
            nvgRoundedRect(vg, rx + 2.0f, ly + 1.0f, rw - 4.0f, lh - 2.0f, 2.0f);
            nvgFillColor(vg, nvgRGBA(c_accent.r, c_accent.g, c_accent.b,
                                      static_cast<std::uint8_t>(c_accent.a * hov_alpha)));
            nvgFill(vg);
        }

        // ── Selected indicator ──
        const bool is_selected = !line.id.empty() && line.id == selectedId_;
        if (is_selected && line.enabled) {
            // Accent bar on the left
            nvgBeginPath(vg);
            nvgRoundedRect(vg, rx + 2.0f, ly + 2.0f, 3.0f, lh - 4.0f, 1.5f);
            nvgFillColor(vg, nvgRGBA(c_accent.r, c_accent.g, c_accent.b,
                                      static_cast<std::uint8_t>(c_accent.a * content_fade)));
            nvgFill(vg);

            // Subtle background tint
            nvgBeginPath(vg);
            nvgRoundedRect(vg, rx + 2.0f, ly + 1.0f, rw - 4.0f, lh - 2.0f, 2.0f);
            nvgFillColor(vg, nvgRGBA(c_accent.r, c_accent.g, c_accent.b,
                                      static_cast<std::uint8_t>(c_accent.a * 0.08f * content_fade)));
            nvgFill(vg);
        }

        // ── Content ──
        // Group headers: chevron at padX, then icon/label after arrow_space
        // Items:         icon/label start at baseline (padX + arrow_space), indented by ip per level
        const float baseline = rx + padX + arrow_space;
        float tx;
        if (line.isGroupHeader) {
            tx = rx + padX;
        } else {
            tx = baseline + ip * static_cast<float>(line.indent);
        }

        if (line.isGroupHeader) {
            // Chevron expand/collapse indicator
            const float ch_y = ly + (lh - ch_s) * 0.5f;
            const SvgIconRef& chev = line.groupExpanded ? Icons::chevron_down : Icons::chevron_right;
            NVGcolor ch_col = nvgRGBA(
                theme.colors.on_surface.r,
                theme.colors.on_surface.g,
                theme.colors.on_surface.b,
                line.enabled ? theme.colors.on_surface.a
                             : theme.colors.on_surface.a / 2);
            Icons::renderIcon(vg, chev, tx, ch_y, ch_s, ch_col);
            tx = baseline;

            // Group icon
            if (line.icon.svg_data) {
                const float ico_y = ly + (lh - icon_s) * 0.5f;
                NVGcolor ico_col = nvgRGBA(
                    c_accent.r, c_accent.g, c_accent.b,
                    line.enabled ? c_accent.a
                                 : static_cast<std::uint8_t>(c_accent.a * 0.4f));
                Icons::renderIcon(vg, line.icon, tx, ico_y, icon_s, ico_col);
                tx += icon_s + 4.0f;
            }

            // Group label — real bold when available, fallback to faux offset.
            nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
            const float text_y = ly + lh * 0.5f;
            nvgFontSize(vg, size);
            NVGcolor group_col = nvgRGBA(theme.colors.on_surface.r,
                                          theme.colors.on_surface.g,
                                          theme.colors.on_surface.b,
                                          line.enabled ? theme.colors.on_surface.a
                                                       : theme.colors.on_surface.a / 2);
            nvgFillColor(vg, group_col);
            if (has_bold) {
                nvgFontFace(vg, bold_face.c_str());
                nvgText(vg, tx, text_y, line.label.c_str(), nullptr);
                nvgFontFace(vg, face.c_str());
            } else {
                nvgText(vg, tx - 0.3f, text_y, line.label.c_str(), nullptr);
                nvgText(vg, tx + 0.3f, text_y, line.label.c_str(), nullptr);
            }

        } else {
            // Item icon (if present) — faded during animation
            if (line.icon.svg_data) {
                const float ico_y = ly + (lh - icon_s) * 0.5f;
                std::uint8_t ico_alpha = static_cast<std::uint8_t>(
                    (line.enabled ? (is_selected ? c_accent.a
                                                  : static_cast<std::uint8_t>(c_accent.a * 0.7f))
                                   : static_cast<std::uint8_t>(c_accent.a * 0.3f)) * content_fade);
                NVGcolor ico_col = nvgRGBA(c_accent.r, c_accent.g, c_accent.b, ico_alpha);
                Icons::renderIcon(vg, line.icon, tx, ico_y, icon_s, ico_col);
                tx += icon_s + 6.0f;
            }

            // Item label — faded during animation
            nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
            const float text_y = ly + lh * 0.5f;
            if (is_selected && line.enabled) {
                nvgFillColor(vg, nvgRGBA(c_accent.r, c_accent.g, c_accent.b,
                                          static_cast<std::uint8_t>(c_accent.a * content_fade)));
            } else {
                nvgFillColor(vg, nvgRGBA(theme.colors.on_surface.r,
                                          theme.colors.on_surface.g,
                                          theme.colors.on_surface.b,
                                          static_cast<std::uint8_t>(
                                              (line.enabled ? theme.colors.on_surface.a
                                                           : theme.colors.on_surface.a / 2) * content_fade)));
            }
            nvgText(vg, tx, text_y, line.label.c_str(), nullptr);
        }
    }

    // ── Compute scrollbar geometry ──
    const float sbSize = 5.0f;
    const float sbGap  = 2.0f;
    const float visH = bounds_.h;
    const float totalContentH = lines_.empty() ? 0.0f
        : (lines_.back().y + lines_.back().h - (bounds_.y - scrollY_));
    const bool needScroll = totalContentH > visH + 1.0f;
    const float maxScroll = needScroll ? (totalContentH - visH) : 0.0f;
    // Clamp scrollY_ to valid range (safety net after rebuildLines)
    scrollY_ = (std::min)(scrollY_, maxScroll);
    if (!needScroll) scrollY_ = 0.0f;

    // ── Overpaint the rightmost 6px to cover any text that bleeds past the widget edge ──
    nvgBeginPath(vg);
    nvgRect(vg, bounds_.x + bounds_.w - 6.0f, bounds_.y, 6.0f, bounds_.h);
    nvgFillColor(vg, nvgRGBA(theme.colors.surface.r,
                              theme.colors.surface.g,
                              theme.colors.surface.b,
                              theme.colors.surface.a));
    nvgFill(vg);

    // ── Resize handle on right edge ──
    {
        const float hx = bounds_.x + bounds_.w - 4.0f;
        const float h_alpha = nearRightEdge_ ? 0.4f : 0.08f;
        nvgBeginPath(vg);
        nvgRoundedRect(vg, hx, bounds_.y + 4.0f, 4.0f, bounds_.h - 8.0f, 1.5f);
        nvgFillColor(vg, nvgRGBA(theme.colors.on_surface.r,
                                  theme.colors.on_surface.g,
                                  theme.colors.on_surface.b,
                                  static_cast<std::uint8_t>(theme.colors.on_surface.a * h_alpha)));
        nvgFill(vg);
    }    // ── Focus ring ──
    if (focused_) {
        drawFocusRing(vg, bounds_, theme);
    }

    // ── Vertical scrollbar ──
    if (needScroll && sbAlpha > 0.01f) {
        const float trackX = bounds_.x + bounds_.w - sbSize - sbGap;
        const float trackY = bounds_.y + sbGap;
        const float th = visH - sbGap * 2.0f;
        const float thumbH = (std::max)(20.0f, th * th / totalContentH);
        const float thumbY = trackY + (scrollY_ / maxScroll) * (th - thumbH);

        // Track
        nvgBeginPath(vg);
        nvgRoundedRect(vg, trackX, trackY, sbSize, th, 2.5f);
        nvgFillColor(vg, nvgRGBA(theme.colors.on_surface.r,
                                  theme.colors.on_surface.g,
                                  theme.colors.on_surface.b,
                                  static_cast<std::uint8_t>(theme.colors.on_surface.a * 0.08f * sbAlpha)));
        nvgFill(vg);

        // Thumb
        nvgBeginPath(vg);
        nvgRoundedRect(vg, trackX, thumbY, sbSize, thumbH, 2.5f);
        nvgFillColor(vg, nvgRGBA(theme.colors.on_surface.r,
                                  theme.colors.on_surface.g,
                                  theme.colors.on_surface.b,
                                  static_cast<std::uint8_t>(theme.colors.on_surface.a * 0.25f * sbAlpha)));
        nvgFill(vg);
    }

    nvgRestore(vg);
}

// ––––––––[ updateHover ] tracks hover state, right-edge proximity for resize cursor, and scrollbar.
void Nav::updateHover(float mx, float my) noexcept {
    const bool inside = containsPoint(mx, my);
    if (!inside) {
        hoveredIdx_ = -1;
        nearRightEdge_ = false;
        scrollbarHovered_ = false;
    } else {
        nearRightEdge_ = inside && (mx >= bounds_.x + bounds_.w - 5.0f && mx <= bounds_.x + bounds_.w);
        if (nearRightEdge_ && window_) {
            window_->setCursorShape(CursorShape::HResize);
        }
        // Check scrollbar hover — resets the wheel fade timer as well
        const float sbSize = 5.0f;
        const float sbGap = 2.0f;
        scrollbarHovered_ = (mx >= bounds_.x + bounds_.w - sbSize - sbGap && mx <= bounds_.x + bounds_.w - sbGap);
        if (scrollbarHovered_) scrollWheelActive_ = true;
    }
}

// ––––––––[ tick ] drives hover, scrollbar fade, and group expand/collapse animations.
void Nav::tick(float dt, const Theme& theme) noexcept {
    hoverAnim_.setTarget(hoveredIdx_ >= 0 ? 1.0f : 0.0f);
    hoverAnim_.tick(dt, theme.metrics.animation.hover_tau);

    // Scroll wheel timer — auto-fade scrollbar after inactivity
    if (scrollWheelTimer_ > 0.0f) {
        scrollWheelTimer_ -= dt;
        if (scrollWheelTimer_ <= 0.0f) scrollWheelActive_ = false;
    }

    scrollAnim_.setTarget(scrollbarHovered_ || scrollDragging_ || scrollWheelActive_ ? 1.0f : 0.0f);
    scrollAnim_.tick(dt, theme.metrics.animation.hover_tau);

    for (auto& g : groups_) {
        g.expandAnim.setTarget(g.expanded ? 1.0f : 0.0f);
        g.expandAnim.tick(dt, theme.metrics.animation.hover_tau * 0.8f);
    }
}

// ––––––––[ handleEvent ] mouse input for selection and expand/collapse.
EventResult Nav::handleEvent(const Event& e, const Window& w) {
    if (!enabled_) return EventResult::Ignored;
    rebuildLines(w.theme());

    float mx{}, my{};
    w.cursorPosition(mx, my);
    const bool inside = containsPoint(mx, my);

    // ── Find which line the cursor is over ──
    auto find_line = [&]() -> int {
        if (!inside) return -1;
        for (std::size_t i = 0; i < lines_.size(); ++i) {
            const auto& l = lines_[i];
            if (my >= l.y && my < l.y + l.h) return static_cast<int>(i);
        }
        return -1;
    };

    switch (kind_of(e)) {
    case EventKind::MouseMove: {
        if (resizing_) {
            const float dx = mx - resizeStartX_;
            const float new_w = (std::max)(minWidth_, resizeOrigW_ + dx);
            bounds_.w = new_w;
            if (on_width_change_) on_width_change_(new_w);
            const_cast<Window&>(w).setCursorShape(CursorShape::HResize);
            return EventResult::Consumed;
        }
        if (scrollDragging_) {
            const float dy = my - scrollDragStartY_;
            const float visH = bounds_.h;
            const float sbGap = 2.0f;
            const float th = visH - sbGap * 2.0f;
            const float totalContentH = lines_.empty() ? 0.0f
                : (lines_.back().y + lines_.back().h - (bounds_.y - scrollY_));
            const float maxScroll = (std::max)(0.0f, totalContentH - visH);
            if (maxScroll > 0.0f) {
                const float thumbH = (std::max)(20.0f, th * th / totalContentH);
                const float dragRange = (std::max)(1.0f, th - thumbH);
                scrollY_ = scrollDragOrigY_ + dy * (maxScroll / dragRange);
                scrollY_ = (std::max)(0.0f, (std::min)(scrollY_, maxScroll));
            }
            return EventResult::Consumed;
        }
        const bool near_edge = inside && (mx >= bounds_.x + bounds_.w - 5.0f && mx <= bounds_.x + bounds_.w);
        if (near_edge) {
            const_cast<Window&>(w).setCursorShape(CursorShape::HResize);
            nearRightEdge_ = true;
        } else {
            if (nearRightEdge_) {
                const_cast<Window&>(w).setCursorShape(CursorShape::Arrow);
            }
            nearRightEdge_ = false;
        }
        const int idx = find_line();
        if (idx != hoveredIdx_) {
            hoveredIdx_ = idx;
        }
        return inside ? EventResult::Consumed : EventResult::Ignored;
    }
    case EventKind::MouseDown: {
        if (inside) {
            // Check scrollbar click first
            const float sbSize = 5.0f;
            const float sbGap = 2.0f;
            const float visH = bounds_.h;
            const float totalContentH = lines_.empty() ? 0.0f
                : (lines_.back().y + lines_.back().h - (bounds_.y - scrollY_));
            const bool needScroll = totalContentH > visH + 1.0f;
            if (needScroll && mx >= bounds_.x + bounds_.w - sbSize - sbGap && mx <= bounds_.x + bounds_.w - sbGap) {
                // Click on scrollbar track — page up/down or start drag
                const float trackY = bounds_.y + sbGap;
                const float th = visH - sbGap * 2.0f;
                const float thumbH = (std::max)(20.0f, th * th / totalContentH);
                // Current thumb position
                const float curThumbY = trackY + (scrollY_ / (std::max)(1.0f, totalContentH - visH)) * (th - thumbH);
                if (my >= curThumbY && my <= curThumbY + thumbH) {
                    // Click on thumb — start dragging
                    scrollDragging_ = true;
                    scrollDragStartY_ = my;
                    scrollDragOrigY_ = scrollY_;
                } else {
                    // Click in track — page up/down by one viewport height
                    const float page = visH * 0.8f;
                    scrollY_ += (my < curThumbY) ? -page : page;
                    scrollY_ = (std::max)(0.0f, (std::min)(scrollY_, totalContentH - visH));
                }
                return EventResult::Consumed;
            }
            const bool on_resize = (mx >= bounds_.x + bounds_.w - 5.0f && mx <= bounds_.x + bounds_.w);
            if (on_resize) {
                resizing_ = true;
                resizeStartX_ = mx;
                resizeOrigW_ = bounds_.w;
                return EventResult::Consumed;
            }
            return EventResult::Consumed;
        }
        return EventResult::Ignored;
    }
    case EventKind::MouseUp: {
        if (resizing_) {
            resizing_ = false;
            const_cast<Window&>(w).setCursorShape(CursorShape::Arrow);
            return EventResult::Consumed;
        }
        if (scrollDragging_) {
            scrollDragging_ = false;
            return EventResult::Consumed;
        }
        if (inside) {
            const int idx = find_line();
            if (idx >= 0 && idx < static_cast<int>(lines_.size())) {
                const auto& line = lines_[static_cast<std::size_t>(idx)];
                if (!line.enabled) return EventResult::Consumed;

                if (line.isGroupHeader) {
                    auto& g = groups_[line.groupIdx];
                    g.expanded = !g.expanded;
                } else if (line.isItem) {
                    if (!line.id.empty()) {
                        setSelectedItem(line.id);
                    }
                    if (line.on_click) {
                        line.on_click();
                    }
                }
            }
            return EventResult::Consumed;
        }
        return EventResult::Ignored;
    }
    // ── Mouse wheel scroll ──
    case EventKind::MouseScroll: {
        if (!inside) return EventResult::Ignored;
        const auto& se = std::get<MouseScrollEvent>(e);
        const float visH = bounds_.h;
        const float totalContentH = lines_.empty() ? 0.0f
            : (lines_.back().y + lines_.back().h - (bounds_.y - scrollY_));
        if (totalContentH > visH + 1.0f) {
            scrollY_ = (std::max)(0.0f, scrollY_ - se.dy * 40.0f);
            const float maxScroll = totalContentH - visH;
            scrollY_ = (std::min)(scrollY_, maxScroll);
            // Show scrollbar briefly on wheel scroll
            scrollWheelActive_ = true;
            scrollWheelTimer_ = 1.5f;
            return EventResult::Consumed;
        }
        return EventResult::Consumed;
    }
    // ── Keyboard navigation ──
    case EventKind::KeyDown: {
        const auto& ke = std::get<KeyDownEvent>(e);
        const std::size_t n = lines_.size();

        // ── Helper: activate the line at given index (same as mouse click) ──
        auto activate_line = [&](int idx) {
            if (idx < 0 || idx >= static_cast<int>(n)) return;
            const auto& line = lines_[static_cast<std::size_t>(idx)];
            if (!line.enabled) return;
            if (line.isGroupHeader) {
                auto& g = groups_[line.groupIdx];
                g.expanded = !g.expanded;
            } else if (line.isItem) {
                if (!line.id.empty()) {
                    setSelectedItem(line.id);
                }
                if (line.on_click) {
                    line.on_click();
                }
            }
        };

        // ── Helper: find the next enabled line (up or down) ──
        auto find_enabled = [&](int start, int dir) -> int {
            for (int i = start + dir; i >= 0 && i < static_cast<int>(n); i += dir) {
                if (lines_[static_cast<std::size_t>(i)].enabled) return i;
            }
            return -1;
        };

        // ── If no line is hovered yet, start from the first enabled line or selected item ──
        if (hoveredIdx_ < 0 || hoveredIdx_ >= static_cast<int>(n)) {
            hoveredIdx_ = -1;
            // Try to find the line matching the selected id
            if (!selectedId_.empty()) {
                for (std::size_t i = 0; i < n; ++i) {
                    if (lines_[i].id == selectedId_ && lines_[i].enabled) {
                        hoveredIdx_ = static_cast<int>(i);
                        break;
                    }
                }
            }
            if (hoveredIdx_ < 0) {
                hoveredIdx_ = find_enabled(-1, 1);
            }
        }

        // ── Helper: scroll to keep the hovered line visible ──
        auto follow_hovered = [&]() {
            if (hoveredIdx_ < 0 || hoveredIdx_ >= static_cast<int>(n)) return;
            const auto& hl = lines_[static_cast<std::size_t>(hoveredIdx_)];
            // hl.y already includes -scrollY_ offset from rebuildLines()
            const float sTop = hl.y;
            const float sBot = hl.y + hl.h;
            if (sTop < bounds_.y) {
                scrollY_ -= (bounds_.y - sTop);
            } else if (sBot > bounds_.y + bounds_.h) {
                scrollY_ += (sBot - (bounds_.y + bounds_.h));
            }
            const float totalContentH = lines_.empty() ? 0.0f
                : (lines_.back().y + lines_.back().h - (bounds_.y - scrollY_));
            const float maxScroll = (std::max)(0.0f, totalContentH - bounds_.h);
            scrollY_ = (std::max)(0.0f, (std::min)(scrollY_, maxScroll));
        };

        switch (ke.key) {
        case GLFW_KEY_UP: {
            const int next = find_enabled(hoveredIdx_, -1);
            if (next >= 0) {
                hoveredIdx_ = next;
                follow_hovered();
                return EventResult::Consumed;
            }
            return EventResult::Consumed;
        }
        case GLFW_KEY_DOWN: {
            const int next = find_enabled(hoveredIdx_, 1);
            if (next >= 0) {
                hoveredIdx_ = next;
                follow_hovered();
                return EventResult::Consumed;
            }
            return EventResult::Consumed;
        }
        case GLFW_KEY_ENTER:
        case GLFW_KEY_SPACE: {
            if (hoveredIdx_ >= 0 && hoveredIdx_ < static_cast<int>(n)) {
                activate_line(hoveredIdx_);
            }
            return EventResult::Consumed;
        }
        case GLFW_KEY_HOME: {
            const int next = find_enabled(-1, 1);
            if (next >= 0) {
                hoveredIdx_ = next;
                follow_hovered();
            }
            return EventResult::Consumed;
        }
        case GLFW_KEY_END: {
            const int next = find_enabled(static_cast<int>(n), -1);
            if (next >= 0) {
                hoveredIdx_ = next;
                follow_hovered();
            }
            return EventResult::Consumed;
        }
        case GLFW_KEY_LEFT: {
            if (hoveredIdx_ >= 0 && hoveredIdx_ < static_cast<int>(n)) {
                const auto& line = lines_[static_cast<std::size_t>(hoveredIdx_)];
                if (line.isGroupHeader) {
                    // Collapse the group
                    auto& g = groups_[line.groupIdx];
                    g.expanded = false;
                } else if (line.isItem && line.indent > 0) {
                    // Navigate up to the group header
                    const int parent_header = find_enabled(hoveredIdx_, -1);
                    if (parent_header >= 0) {
                        const auto& pl = lines_[static_cast<std::size_t>(parent_header)];
                        if (pl.isGroupHeader) {
                            hoveredIdx_ = parent_header;
                        }
                    }
                }
            }
            return EventResult::Consumed;
        }
        case GLFW_KEY_RIGHT: {
            if (hoveredIdx_ >= 0 && hoveredIdx_ < static_cast<int>(n)) {
                const auto& line = lines_[static_cast<std::size_t>(hoveredIdx_)];
                if (line.isGroupHeader) {
                    auto& g = groups_[line.groupIdx];
                    if (!g.expanded) {
                        // Expand the group
                        g.expanded = true;
                    } else {
                        // Already expanded: go to first child item
                        const int first_child = hoveredIdx_ + 1;
                        if (first_child < static_cast<int>(n)) {
                            const auto& child = lines_[static_cast<std::size_t>(first_child)];
                            if (child.isItem && child.enabled) {
                                hoveredIdx_ = first_child;
                            }
                        }
                    }
                }
            }
            return EventResult::Consumed;
        }
        default:
            return EventResult::Ignored;
        }
    }
    default:
        return EventResult::Ignored;
    }
}

} // namespace fluffy
