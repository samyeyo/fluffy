// src/simple_widget.cpp
// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
// SimpleWidget — beginner custom-widget base backed by a tiny Painter.
//
// MIT License - Copyright (c) 2026 - Samir Tine
// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

#include <fluffy/simple_widget.hpp>

#include <cstring>

#include <nanovg.h>

#include <fluffy/window.hpp>

namespace fluffy {

// ––––––––[ rect ] fills an unrounded rectangle with a color
void Painter::rect(Rect r, Color c) const {
    if (vg_ == nullptr) {
        return;
    }
    nvgBeginPath(vg_);
    nvgRect(vg_, r.x, r.y, r.w, r.h);
    nvgFillColor(vg_, nvgRGBA(c.r, c.g, c.b, c.a));
    nvgFill(vg_);
}

// ––––––––[ roundedRect ] fills a rounded rectangle with a color
void Painter::roundedRect(Rect r, float radius, Color c) const {
    if (vg_ == nullptr) {
        return;
    }
    nvgBeginPath(vg_);
    nvgRoundedRect(vg_, r.x, r.y, r.w, r.h, radius);
    nvgFillColor(vg_, nvgRGBA(c.r, c.g, c.b, c.a));
    nvgFill(vg_);
}

// ––––––––[ ring ] strokes a rounded rectangle outline
void Painter::ring(Rect r, float radius, Color c, float width) const {
    if (vg_ == nullptr) {
        return;
    }
    nvgBeginPath(vg_);
    nvgRoundedRect(vg_, r.x, r.y, r.w, r.h, radius);
    nvgStrokeColor(vg_, nvgRGBA(c.r, c.g, c.b, c.a));
    nvgStrokeWidth(vg_, width);
    nvgStroke(vg_);
}

// ––––––––[ text ] draws left-aligned text inside bounds
void Painter::text(std::string_view s, Color c, float size) const {
    if (vg_ == nullptr || s.empty()) {
        return;
    }
    const float resolved = size > 0.0f ? size : theme_.metrics.text_size;
    nvgFontSize(vg_, resolved);
    nvgFontFace(vg_, "default");
    nvgFillColor(vg_, nvgRGBA(c.r, c.g, c.b, c.a));
    nvgTextAlign(vg_, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
    std::string owned(s);
    nvgText(vg_, bounds_.x + 8.0f, bounds_.y + bounds_.h * 0.5f, owned.c_str(), nullptr);
}

// ––––––––[ onPress ] return true when a mouse press is consumed
bool SimpleWidget::onPress(float x, float y) {
    (void)x;
    (void)y;
    return false;
}

// ––––––––[ preferredWidth ] size hint used when autoSize is enabled
float SimpleWidget::preferredWidth(const Theme& theme, const Window& w) const {
    (void)theme;
    (void)w;
    return 120.0f;
}

// ––––––––[ preferredHeight ] height hint used when autoSize is enabled
float SimpleWidget::preferredHeight(const Theme& theme, const Window& w) const {
    (void)theme;
    (void)w;
    return 40.0f;
}

// ––––––––[ draw ] adapts paint() to the Widget interface
void SimpleWidget::draw(NVGcontext* vg, const Theme& theme, const Window& w) {
    (void)w;
    Painter p(vg, theme, bounds());
    paint(p);
}

// ––––––––[ handleEvent ] routes mouse presses to onPress()
EventResult SimpleWidget::handleEvent(const Event& e, const Window& w) {
    (void)w;
    if (const auto* down = std::get_if<MouseDownEvent>(&e)) {
        if (containsPoint(down->x, down->y)) {
            return onPress(down->x, down->y) ? EventResult::Consumed : EventResult::Ignored;
        }
    }
    return EventResult::Ignored;
}

// ––––––––[ naturalWidth ] forwards layout measuring to preferredWidth()
float SimpleWidget::naturalWidth(const Theme& theme, const Window& w) const {
    return preferredWidth(theme, w);
}

// ––––––––[ naturalHeight ] forwards layout measuring to preferredHeight()
float SimpleWidget::naturalHeight(const Theme& theme, const Window& w) const {
    return preferredHeight(theme, w);
}

} // namespace fluffy
