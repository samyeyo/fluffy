// src/listview.cpp
// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
// ListView — draw, handleEvent, scroll, selection
// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
#include <fluffy/listview.hpp>

#include <algorithm>
#include <cmath>
#include <iterator>

#include <nanovg.h>

#include <GLFW/glfw3.h>

#include <fluffy/window.hpp>
#include <fluffy/yaml_fwd.hpp>

namespace fluffy {

// ––––––––[ ListView ] constructs
ListView::ListView(Rect bounds)
    : Widget(bounds) {}

// ––––––––[ setItems ] replaces
void ListView::setItems(std::vector<std::string> items) {
    items_ = items;
    inlineItems_.clear();
    inlineItems_.reserve(items.size());
    for (auto& s : items) inlineItems_.push_back(Inline{} << s);
    if (selected_ >= std::ssize(items_)) selected_ = -1;
    scroll_y_ = 0.0f;
    hovered_idx_ = -1;
}

// ––––––––[ setItemsInline ] replaces inline
void ListView::setItemsInline(std::vector<Inline> items) {
    inlineItems_ = std::move(items);
    items_.clear();
    items_.reserve(inlineItems_.size());
    for (auto& inl : inlineItems_) {
        std::string plain;
        for (auto& r : inl.runs()) if (r.type == Inline::Run::Type::Text) plain += r.utf8;
        items_.push_back(std::move(plain));
    }
    if (selected_ >= std::ssize(items_)) selected_ = -1;
    scroll_y_ = 0.0f;
    hovered_idx_ = -1;
}

// ––––––––[ selectedInline ] returns Inline
Inline ListView::selectedInline() const noexcept {
    if (selected_ >= 0 && selected_ < std::ssize(inlineItems_)) return inlineItems_[static_cast<std::size_t>(selected_)];
    return Inline{};
}

// ––––––––[ setSelected ] selects
void ListView::setSelected(int idx) {
    if (idx < -1 || idx >= std::ssize(items_)) idx = -1;
    if (idx == selected_) return;
    selected_ = idx;
    if (selected_ >= 0 && on_select_) on_select_(selected_, items_[static_cast<std::size_t>(selected_)]);
}

// ––––––––[ row_h ] row height
float ListView::row_h(const Theme& theme) const noexcept {
    const float size = textSize_ > 0.0f ? textSize_ : theme.metrics.text_size;
    return size + theme.metrics.input_padding_y * 2.0f + 4.0f;
}

// ––––––––[ naturalWidth ] widest item
float ListView::naturalWidth(const Theme& theme, const Window& w) const {
    NVGcontext* vg = w.nvgContext();
    if (!vg) return 160.0f;
    const float size = textSize_ > 0.0f ? textSize_ : theme.metrics.text_size;
    std::string spec = font_.empty() ? theme.font_family : font_;
    std::string face;
    const_cast<Window&>(w).prepareFont(vg, spec, face);
    nvgFontSize(vg, size);
    nvgFontFace(vg, face.c_str());
    float max_w = 0.0f;
    const float icon_gain = theme.metrics.animation.icon_size_gain;
    const float icon_pad = theme.metrics.animation.icon_padding;
    if (!inlineItems_.empty()) {
        for (auto& inl : inlineItems_) {
            if (inl.empty()) continue;
            float tw = 0.0f;
            for (auto& r : inl.runs()) {
                if (r.type == Inline::Run::Type::SvgIcon) {
                    float rs = r.size > 0.0f ? r.size : size;
                    tw += rs * icon_gain + 2.0f * rs * icon_gain * icon_pad + 4.0f;
                } else {
                    float rs = r.size > 0.0f ? r.size : size;
                    nvgFontSize(vg, rs);
                    tw += nvgTextBounds(vg, 0, 0, r.utf8.c_str(), nullptr, nullptr);
                    nvgFontSize(vg, size);
                }
            }
            max_w = std::max(max_w, tw);
        }
    } else {
        for (auto& s : items_) max_w = std::max(max_w, nvgTextBounds(vg, 0, 0, s.c_str(), nullptr, nullptr));
    }
    return max_w + theme.metrics.input_padding_x * 2.0f + 24.0f;
}

// ––––––––[ naturalHeight ] visible rows * row_h
float ListView::naturalHeight(const Theme& theme, const Window&) const {
    const int n = static_cast<int>(std::ssize(items_));
    if (n == 0) return row_h(theme) * 3.0f;
    return row_h(theme) * std::min(n, 8) + 4.0f;
}

// ––––––––[ ensureVisible ] scroll to show idx
void ListView::ensureVisible(int idx, const Theme& theme) {
    if (idx < 0) return;
    float rh = row_h(theme);
    float y = static_cast<float>(idx) * rh;
    if (y < scroll_y_) scroll_y_ = y;
    else if (y + rh > scroll_y_ + bounds_.h) scroll_y_ = y + rh - bounds_.h;
    float max = std::max(0.0f, static_cast<float>(items_.size()) * rh - bounds_.h);
    scroll_y_ = std::clamp(scroll_y_, 0.0f, max);
}

// ––––––––[ draw ] renders list
void ListView::draw(NVGcontext* vg, const Theme& theme, const Window& w) {
    if (!visible_) return;
    float rh = row_h(theme);
    // Background
    nvgBeginPath(vg);
    nvgRoundedRect(vg, bounds_.x, bounds_.y, bounds_.w, bounds_.h, theme.metrics.corner_radius);
    nvgFillColor(vg, nvgRGBA(theme.colors.surface.r, theme.colors.surface.g, theme.colors.surface.b, theme.colors.surface.a));
    nvgFill(vg);
    nvgStrokeWidth(vg, theme.metrics.border_width);
    Color br = focused_ ? theme.colors.accent : theme.colors.border;
    nvgStrokeColor(vg, nvgRGBA(br.r, br.g, br.b, br.a));
    nvgStroke(vg);
    if (focused_) drawFocusRing(vg, bounds_, theme);
    // Clip
    nvgSave(vg);
    nvgIntersectScissor(vg, bounds_.x, bounds_.y, bounds_.w, bounds_.h);
    const float size = textSize_ > 0.0f ? textSize_ : theme.metrics.text_size;
    std::string spec = font_.empty() ? theme.font_family : font_;
    std::string face;
    w.prepareFont(vg, spec, face);
    nvgFontSize(vg, size);
    nvgFontFace(vg, face.c_str());
    nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
    const int count = static_cast<int>(std::ssize(items_));
    int first = static_cast<int>(std::floor(scroll_y_ / rh));
    int last = static_cast<int>(std::ceil((scroll_y_ + bounds_.h) / rh));
    first = std::clamp(first, 0, count);
    last = std::clamp(last, 0, count);
    for (int i = first; i < last; ++i) {
        float y = bounds_.y - scroll_y_ + static_cast<float>(i) * rh;
        // Hover / selected bg
        bool is_sel = i == selected_;
        bool is_hov = i == hovered_idx_;
        if (is_sel) {
            nvgBeginPath(vg);
            nvgRoundedRect(vg, bounds_.x + 2, y + 1, bounds_.w - 4, rh - 2, 2.0f);
            nvgFillColor(vg, nvgRGBA(theme.colors.accent.r, theme.colors.accent.g, theme.colors.accent.b, theme.colors.accent.a));
            nvgFill(vg);
        } else if (is_hov) {
            nvgBeginPath(vg);
            nvgRoundedRect(vg, bounds_.x + 2, y + 1, bounds_.w - 4, rh - 2, 2.0f);
            nvgFillColor(vg, nvgRGBA(theme.colors.accent.r, theme.colors.accent.g, theme.colors.accent.b, 60));
            nvgFill(vg);
        }
        float tx = bounds_.x + theme.metrics.input_padding_x + 4;
        float ty = y + rh * 0.5f;
        if (i < std::ssize(inlineItems_) && !inlineItems_[static_cast<std::size_t>(i)].empty()) {
            Color def = is_sel ? Color{255, 255, 255, 255} : theme.colors.on_surface;
            // Clip inline to row width
            nvgSave(vg);
            nvgIntersectScissor(vg, bounds_.x + 2, y + 1, bounds_.w - 4, rh - 2);
            renderInline(vg, inlineItems_[static_cast<std::size_t>(i)].runs(), tx, ty,
                         NVG_ALIGN_LEFT, NVG_ALIGN_MIDDLE, size,
                         theme.metrics.animation.icon_size_gain, theme.metrics.animation.icon_padding,
                         face.c_str(), def, def);
            nvgRestore(vg);
        } else {
            Color tc = is_sel ? Color{255, 255, 255, 255} : theme.colors.on_surface;
            nvgFillColor(vg, nvgRGBA(tc.r, tc.g, tc.b, tc.a));
            nvgText(vg, tx, ty, items_[static_cast<std::size_t>(i)].c_str(), nullptr);
        }
    }
    nvgRestore(vg);
    // Scrollbar
    float total_h = static_cast<float>(items_.size()) * rh;
    if (total_h > bounds_.h + 1.0f) {
        float th = bounds_.h * (bounds_.h / total_h);
        th = std::max(12.0f, th);
        float ty = bounds_.y + (scroll_y_ / (total_h - bounds_.h)) * (bounds_.h - th);
        nvgBeginPath(vg);
        nvgRoundedRect(vg, bounds_.x + bounds_.w - 5, ty, 3, th, 1.5f);
        nvgFillColor(vg, nvgRGBA(128, 128, 128, 100));
        nvgFill(vg);
    }
}

// ––––––––[ updateHover ] tracks hovered row
void ListView::updateHover(float mx, float my) noexcept {
    if (!containsPoint(mx, my)) { hovered_idx_ = -1; return; }
    float rh = 12.0f; // fallback; tick will use theme, but hover uses approximate
    // Use fixed estimate: will be corrected on next draw tick with theme, but okay
    // Better to compute with default size 14
    rh = 14.0f + 6.0f * 2.0f + 4.0f;
    float y = my - bounds_.y + scroll_y_;
    int idx = static_cast<int>(std::floor(y / rh));
    if (idx < 0 || idx >= std::ssize(items_)) hovered_idx_ = -1;
    else hovered_idx_ = idx;
}

// ––––––––[ tick ] anim
void ListView::tick(float dt, const Theme& theme) noexcept {
    float target = hovered_idx_ >= 0 ? 1.0f : 0.0f;
    hover_anim_.setTarget(target);
    hover_anim_.tick(dt, theme.metrics.animation.hover_tau);
}

// ––––––––[ setFocused ] focus
void ListView::setFocused(bool f) noexcept { focused_ = f; }

// ––––––––[ handleEvent ] selection + scroll
EventResult ListView::handleEvent(const Event& e, const Window& w) {
    if (!enabled_) return EventResult::Ignored;
    float mx{}, my{};
    w.cursorPosition(mx, my);
    bool inside = containsPoint(mx, my);
    switch (kind_of(e)) {
    case EventKind::MouseMove: {
        if (inside) {
            float rh = row_h(w.theme());
            float y = my - bounds_.y + scroll_y_;
            int idx = static_cast<int>(std::floor(y / rh));
            hovered_idx_ = (idx >= 0 && idx < std::ssize(items_)) ? idx : -1;
        } else hovered_idx_ = -1;
        return EventResult::Ignored;
    }
    case EventKind::MouseDown:
        if (inside && window_) window_->requestFocus(this);
        return inside ? EventResult::Consumed : EventResult::Ignored;
    case EventKind::MouseUp: {
        if (inside) {
            float rh = row_h(w.theme());
            float y = my - bounds_.y + scroll_y_;
            int idx = static_cast<int>(std::floor(y / rh));
            if (idx >= 0 && idx < std::ssize(items_)) {
                setSelected(idx);
                ensureVisible(idx, w.theme());
                return EventResult::Consumed;
            }
        }
        return inside ? EventResult::Consumed : EventResult::Ignored;
    }
    case EventKind::MouseScroll: {
        if (!inside) return EventResult::Ignored;
        const auto& se = std::get<MouseScrollEvent>(e);
        scroll_y_ -= se.dy * 20.0f;
        float rh = row_h(w.theme());
        float max = std::max(0.0f, static_cast<float>(items_.size()) * rh - bounds_.h);
        scroll_y_ = std::clamp(scroll_y_, 0.0f, max);
        return EventResult::Consumed;
    }
    case EventKind::KeyDown: {
        if (!focused_) return EventResult::Ignored;
        const auto& ke = std::get<KeyDownEvent>(e);
        if (ke.key == GLFW_KEY_UP) {
            if (selected_ > 0) { setSelected(selected_ - 1); ensureVisible(selected_, w.theme()); }
            return EventResult::Consumed;
        } else if (ke.key == GLFW_KEY_DOWN) {
            if (selected_ + 1 < std::ssize(items_)) { setSelected(selected_ + 1); ensureVisible(selected_, w.theme()); }
            return EventResult::Consumed;
        } else if (ke.key == GLFW_KEY_HOME) {
            if (!items_.empty()) { setSelected(0); ensureVisible(0, w.theme()); }
            return EventResult::Consumed;
        } else if (ke.key == GLFW_KEY_END) {
            if (!items_.empty()) { setSelected(static_cast<int>(std::ssize(items_)) - 1); ensureVisible(selected_, w.theme()); }
            return EventResult::Consumed;
        }
        break;
    }
    default: break;
    }
    return EventResult::Ignored;
}

// ––––––––[ setProperty ] yaml
void ListView::setProperty(std::string_view name, const YamlNode& value) {
    if (name == "selected") { setSelected(value.asInt()); return; }
    if (name == "items") {
        std::vector<std::string> out;
        if (value.IsSequence()) for (const auto& ch : value) out.push_back(ch.asString());
        setItems(std::move(out)); return;
    }
    this->Widget::setProperty(name, value);
}

}  // namespace fluffy
