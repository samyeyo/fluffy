// src/tabview.cpp
// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
// TabView — header + content container
// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
#include <fluffy/tabview.hpp>

#include <algorithm>
#include <iterator>

#include <nanovg.h>

#include <GLFW/glfw3.h>

#include <fluffy/container.hpp>
#include <fluffy/window.hpp>
#include <fluffy/yaml_fwd.hpp>

namespace fluffy {

// ––––––––[ TabView ] constructs
TabView::TabView(Rect bounds)
    : Widget(bounds) {
    content_ = std::make_unique<Container>(Rect{});
    auto vl = std::make_unique<VerticalLayout>();
    vl->setPadding(8.0f);
    content_->setLayout(std::move(vl));
}

// ––––––––[ addTab ] appends
void TabView::addTab(std::string label, std::string id) {
    if (id.empty()) id = label;
    tabs_.push_back({std::move(label), std::move(id)});
    if (selected_ == -1) selected_ = 0;
}

// ––––––––[ setSelected ] selects
void TabView::setSelected(int idx) {
    if (idx < 0 || idx >= std::ssize(tabs_)) return;
    if (idx == selected_) return;
    selected_ = idx;
    if (on_select_) on_select_(selected_, tabs_[static_cast<std::size_t>(selected_)].id);
}

// ––––––––[ rebuildRects ] computes header rects
void TabView::rebuildRects(const Theme& theme, const Window& w) {
    tab_rects_.clear();
    if (tabs_.empty()) return;
    NVGcontext* vg = w.nvgContext();
    const float size = textSize_ > 0.0f ? textSize_ : theme.metrics.text_size;
    float x = bounds_.x;
    float y = bounds_.y;
    float h = size + theme.metrics.button_padding_y * 2.0f;
    for (auto& t : tabs_) {
        float tw = 80.0f;
        if (vg) {
            std::string spec = font_.empty() ? theme.font_family : font_;
            std::string face;
            const_cast<Window&>(w).prepareFont(vg, spec, face);
            nvgFontSize(vg, size);
            nvgFontFace(vg, face.c_str());
            tw = nvgTextBounds(vg, 0, 0, t.label.c_str(), nullptr, nullptr) + theme.metrics.button_padding_x * 2.0f + 8.0f;
        }
        tab_rects_.push_back(Rect{x, y, tw, h});
        x += tw + 4.0f;
    }
}

// ––––––––[ naturalWidth ] max tab row
float TabView::naturalWidth(const Theme& theme, const Window& w) const {
    const_cast<TabView*>(this)->rebuildRects(theme, w);
    float total = 0.0f;
    for (auto& r : tab_rects_) total += r.w + 4.0f;
    if (content_) total = std::max(total, content_->naturalWidth(theme, w));
    return total + 16.0f;
}

// ––––––––[ naturalHeight ] header + content
float TabView::naturalHeight(const Theme& theme, const Window& w) const {
    const float size = textSize_ > 0.0f ? textSize_ : theme.metrics.text_size;
    float header_h = size + theme.metrics.button_padding_y * 2.0f;
    float ch = 0.0f;
    if (content_) ch = content_->naturalHeight(theme, w);
    return header_h + (ch > 0 ? ch : 80.0f);
}

// ––––––––[ setWindow ] propagates to content
void TabView::setWindow(Window* w) noexcept {
    window_ = w;
    if (content_) content_->setWindow(w);
}

// ––––––––[ draw ] header tabs + content
void TabView::draw(NVGcontext* vg, const Theme& theme, const Window& w) {
    if (!visible_) return;
    rebuildRects(theme, w);
    const float header_h = tab_rects_.empty() ? 0.0f : tab_rects_[0].h;
    // TabView background — single surface for header + content
    nvgBeginPath(vg);
    nvgRoundedRect(vg, bounds_.x, bounds_.y, bounds_.w, bounds_.h, theme.metrics.corner_radius);
    nvgFillColor(vg, nvgRGBA(theme.colors.surface.r, theme.colors.surface.g, theme.colors.surface.b, theme.colors.surface.a));
    nvgFill(vg);
    // Header background — same surface
    nvgBeginPath(vg);
    nvgRoundedRect(vg, bounds_.x, bounds_.y, bounds_.w, header_h, 4.0f);
    nvgFillColor(vg, nvgRGBA(theme.colors.surface.r, theme.colors.surface.g, theme.colors.surface.b, theme.colors.surface.a));
    nvgFill(vg);
    // Tabs
    const float size = textSize_ > 0.0f ? textSize_ : theme.metrics.text_size;
    std::string spec = font_.empty() ? theme.font_family : font_;
    std::string face;
    w.prepareFont(vg, spec, face);
    nvgFontSize(vg, size);
    nvgFontFace(vg, face.c_str());
    nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
    for (size_t i = 0; i < tabs_.size(); ++i) {
        Rect r = tab_rects_[i];
        bool sel = selected_ == static_cast<int>(i);
        bool hov = hovered_  == static_cast<int>(i);
        if (sel) {
            nvgFillColor(vg, nvgRGBA(accentColor(theme).r, accentColor(theme).g, accentColor(theme).b, accentColor(theme).a));
        } else if (hov) {
            nvgBeginPath(vg);
            nvgRoundedRect(vg, r.x, r.y, r.w, r.h, 4.0f);
            nvgFillColor(vg, nvgRGBA(theme.colors.accent.r, theme.colors.accent.g, theme.colors.accent.b, 40));
            nvgFill(vg);
            nvgFillColor(vg, nvgRGBA(theme.colors.on_surface.r, theme.colors.on_surface.g, theme.colors.on_surface.b, theme.colors.on_surface.a));
        } else {
            nvgFillColor(vg, nvgRGBA(theme.colors.on_surface.r, theme.colors.on_surface.g, theme.colors.on_surface.b, theme.colors.on_surface.a));
        }
        nvgText(vg, r.x + r.w * 0.5f, r.y + r.h * 0.5f, tabs_[i].label.c_str(), nullptr);
    }
    // Content area — same background as TabView for visual consistency
    float content_y = bounds_.y + header_h;
    float content_h = bounds_.h - header_h;
    // Fill content background with same surface as header/tab bar
    nvgBeginPath(vg);
    nvgRoundedRect(vg, bounds_.x, content_y, bounds_.w, std::max(0.0f, content_h), theme.metrics.corner_radius);
    nvgFillColor(vg, nvgRGBA(theme.colors.surface.r, theme.colors.surface.g, theme.colors.surface.b, theme.colors.surface.a));
    nvgFill(vg);
    if (content_) {
        content_->setBounds(Rect{bounds_.x, content_y, bounds_.w, std::max(0.0f, content_h)});
        content_->draw(vg, theme, w);
    }
    // Border — halo like TextInput when focused (around panel + selected tab)
    if (focused_ && selected_ >= 0 && selected_ < std::ssize(tab_rects_)) {
        const Color ac = accentColor(theme);
        const float r = theme.metrics.corner_radius;
        const float rTab = 4.0f;
        Rect rs = tab_rects_[static_cast<size_t>(selected_)];
        // Clamp tab rect to bounds for safety
        float cr = std::min(rTab, std::min(rs.w, rs.h) * 0.5f);
        float contentR = r;
        const bool isLeftmost = rs.x <= bounds_.x + 0.5f;
        nvgBeginPath(vg);
        if (isLeftmost) {
            // Leftmost tab — left edge is flush with panel, avoid double corner at top-left
            nvgMoveTo(vg, rs.x + cr, rs.y);
            nvgLineTo(vg, rs.x + rs.w - cr, rs.y);
            nvgArcTo(vg, rs.x + rs.w, rs.y, rs.x + rs.w, rs.y + cr, cr);
            nvgLineTo(vg, rs.x + rs.w, content_y);
            nvgLineTo(vg, bounds_.x + bounds_.w - contentR, content_y);
            nvgArcTo(vg, bounds_.x + bounds_.w, content_y, bounds_.x + bounds_.w, content_y + contentR, contentR);
            nvgLineTo(vg, bounds_.x + bounds_.w, bounds_.y + bounds_.h - contentR);
            nvgArcTo(vg, bounds_.x + bounds_.w, bounds_.y + bounds_.h, bounds_.x + bounds_.w - contentR, bounds_.y + bounds_.h, contentR);
            nvgLineTo(vg, bounds_.x + contentR, bounds_.y + bounds_.h);
            nvgArcTo(vg, bounds_.x, bounds_.y + bounds_.h, bounds_.x, bounds_.y + bounds_.h - contentR, contentR);
            nvgLineTo(vg, bounds_.x, rs.y + cr);
            nvgArcTo(vg, bounds_.x, rs.y, bounds_.x + cr, rs.y, cr);
        } else {
            nvgMoveTo(vg, rs.x + cr, rs.y);
            nvgLineTo(vg, rs.x + rs.w - cr, rs.y);
            nvgArcTo(vg, rs.x + rs.w, rs.y, rs.x + rs.w, rs.y + cr, cr);
            nvgLineTo(vg, rs.x + rs.w, content_y);
            // Top edge of content to right of tab
            nvgLineTo(vg, bounds_.x + bounds_.w - contentR, content_y);
            nvgArcTo(vg, bounds_.x + bounds_.w, content_y, bounds_.x + bounds_.w, content_y + contentR, contentR);
            nvgLineTo(vg, bounds_.x + bounds_.w, bounds_.y + bounds_.h - contentR);
            nvgArcTo(vg, bounds_.x + bounds_.w, bounds_.y + bounds_.h, bounds_.x + bounds_.w - contentR, bounds_.y + bounds_.h, contentR);
            nvgLineTo(vg, bounds_.x + contentR, bounds_.y + bounds_.h);
            nvgArcTo(vg, bounds_.x, bounds_.y + bounds_.h, bounds_.x, bounds_.y + bounds_.h - contentR, contentR);
            nvgLineTo(vg, bounds_.x, content_y + contentR);
            nvgArcTo(vg, bounds_.x, content_y, bounds_.x + contentR, content_y, contentR);
            nvgLineTo(vg, rs.x, content_y);
            nvgLineTo(vg, rs.x, rs.y + cr);
            nvgArcTo(vg, rs.x, rs.y, rs.x + cr, rs.y, cr);
        }
        nvgClosePath(vg);
        nvgStrokeWidth(vg, theme.metrics.input_focus_width);
        nvgStrokeColor(vg, nvgRGBA(ac.r, ac.g, ac.b, ac.a));
        nvgStroke(vg);
    } else if (focused_) {
        const Color ac = accentColor(theme);
        nvgBeginPath(vg);
        nvgRoundedRect(vg, bounds_.x, bounds_.y, bounds_.w, bounds_.h, theme.metrics.corner_radius);
        nvgStrokeWidth(vg, theme.metrics.input_focus_width);
        nvgStrokeColor(vg, nvgRGBA(ac.r, ac.g, ac.b, ac.a));
        nvgStroke(vg);
    } else if (selected_ >= 0 && selected_ < std::ssize(tab_rects_)) {
        // Selection hot line at first view — same shape as focus halo, accent even when unfocused
        const Color ac = accentColor(theme);
        const float r = theme.metrics.corner_radius;
        const float rTab = 4.0f;
        Rect rs = tab_rects_[static_cast<size_t>(selected_)];
        float cr = std::min(rTab, std::min(rs.w, rs.h) * 0.5f);
        float contentR = r;
        const bool isLeftmost = rs.x <= bounds_.x + 0.5f;
        nvgBeginPath(vg);
        if (isLeftmost) {
            nvgMoveTo(vg, rs.x + cr, rs.y);
            nvgLineTo(vg, rs.x + rs.w - cr, rs.y);
            nvgArcTo(vg, rs.x + rs.w, rs.y, rs.x + rs.w, rs.y + cr, cr);
            nvgLineTo(vg, rs.x + rs.w, content_y);
            nvgLineTo(vg, bounds_.x + bounds_.w - contentR, content_y);
            nvgArcTo(vg, bounds_.x + bounds_.w, content_y, bounds_.x + bounds_.w, content_y + contentR, contentR);
            nvgLineTo(vg, bounds_.x + bounds_.w, bounds_.y + bounds_.h - contentR);
            nvgArcTo(vg, bounds_.x + bounds_.w, bounds_.y + bounds_.h, bounds_.x + bounds_.w - contentR, bounds_.y + bounds_.h, contentR);
            nvgLineTo(vg, bounds_.x + contentR, bounds_.y + bounds_.h);
            nvgArcTo(vg, bounds_.x, bounds_.y + bounds_.h, bounds_.x, bounds_.y + bounds_.h - contentR, contentR);
            nvgLineTo(vg, bounds_.x, rs.y + cr);
            nvgArcTo(vg, bounds_.x, rs.y, bounds_.x + cr, rs.y, cr);
        } else {
            nvgMoveTo(vg, rs.x + cr, rs.y);
            nvgLineTo(vg, rs.x + rs.w - cr, rs.y);
            nvgArcTo(vg, rs.x + rs.w, rs.y, rs.x + rs.w, rs.y + cr, cr);
            nvgLineTo(vg, rs.x + rs.w, content_y);
            nvgLineTo(vg, bounds_.x + bounds_.w - contentR, content_y);
            nvgArcTo(vg, bounds_.x + bounds_.w, content_y, bounds_.x + bounds_.w, content_y + contentR, contentR);
            nvgLineTo(vg, bounds_.x + bounds_.w, bounds_.y + bounds_.h - contentR);
            nvgArcTo(vg, bounds_.x + bounds_.w, bounds_.y + bounds_.h, bounds_.x + bounds_.w - contentR, bounds_.y + bounds_.h, contentR);
            nvgLineTo(vg, bounds_.x + contentR, bounds_.y + bounds_.h);
            nvgArcTo(vg, bounds_.x, bounds_.y + bounds_.h, bounds_.x, bounds_.y + bounds_.h - contentR, contentR);
            nvgLineTo(vg, bounds_.x, content_y + contentR);
            nvgArcTo(vg, bounds_.x, content_y, bounds_.x + contentR, content_y, contentR);
            nvgLineTo(vg, rs.x, content_y);
            nvgLineTo(vg, rs.x, rs.y + cr);
            nvgArcTo(vg, rs.x, rs.y, rs.x + cr, rs.y, cr);
        }
        nvgClosePath(vg);
        nvgStrokeWidth(vg, theme.metrics.input_focus_width);
        nvgStrokeColor(vg, nvgRGBA(ac.r, ac.g, ac.b, ac.a));
        nvgStroke(vg);
    } else {
        nvgBeginPath(vg);
        nvgRoundedRect(vg, bounds_.x, bounds_.y, bounds_.w, bounds_.h, theme.metrics.corner_radius);
        nvgStrokeWidth(vg, theme.metrics.border_width);
        nvgStrokeColor(vg, nvgRGBA(theme.colors.border.r, theme.colors.border.g, theme.colors.border.b, theme.colors.border.a));
        nvgStroke(vg);
    }
}

// ––––––––[ updateHover ] hovered tab
void TabView::updateHover(float mx, float my) noexcept {
    hovered_ = -1;
    for (size_t i = 0; i < tab_rects_.size(); ++i) {
        if (tab_rects_[i].containsPoint(mx, my)) { hovered_ = static_cast<int>(i); break; }
    }
    if (content_) content_->updateHover(mx, my);
}

// ––––––––[ tick ] propagate
void TabView::tick(float dt, const Theme& theme) noexcept {
    if (content_) content_->tick(dt, theme);
}

// ––––––––[ handleEvent ] tab click + delegate to content
EventResult TabView::handleEvent(const Event& e, const Window& w) {
    if (!enabled_) return EventResult::Ignored;
    float mx{}, my{};
    w.cursorPosition(mx, my);
    if (kind_of(e) == EventKind::MouseDown) {
        for (size_t i = 0; i < tab_rects_.size(); ++i) {
            if (tab_rects_[i].containsPoint(mx, my)) {
                if (window_) window_->requestFocus(this);
                setSelected(static_cast<int>(i));
                return EventResult::Consumed;
            }
        }
    }
    if (content_) {
        float header_h = tab_rects_.empty() ? 0 : tab_rects_[0].h;
        if (my >= bounds_.y + header_h) {
            return content_->handleEvent(e, w);
        }
    }
    return EventResult::Ignored;
}

// ––––––––[ setProperty ] yaml
void TabView::setProperty(std::string_view name, const YamlNode& value) {
    if (name == "selected") { setSelected(value.asInt()); return; }
    if (name == "tabs") {
        if (value.IsSequence()) {
            tabs_.clear();
            for (const auto& ch : value) addTab(ch.asString());
        }
        return;
    }
    // Fallback: ignore unknown
    (void)name; (void)value;
}

}  // namespace fluffy
