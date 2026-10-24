// include/fluffy/simple_widget.hpp
// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
// SimpleWidget — beginner base for custom widgets without NanoVG plumbing.
//
// MIT License - Copyright (c) 2026 - Samir Tine
// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

#pragma once
#include <fluffy/widget.hpp>

#include <string>

#include <nanovg.h>

#include <fluffy/geom.hpp>
#include <fluffy/theme.hpp>

namespace fluffy {

// ––––––––[ Painter ] tiny NanoVG wrapper for custom widget painting
class Painter {
public:
    // ––––––––[ Painter ] wraps a NanoVG context and theme for one frame
    Painter(NVGcontext* vg, const Theme& theme, Rect bounds) : vg_(vg), theme_(theme), bounds_(bounds) {}
    // ––––––––[ rect ] fills an unrounded rectangle with a color
    void rect(Rect r, Color c) const;
    // ––––––––[ roundedRect ] fills a rounded rectangle with a color
    void roundedRect(Rect r, float radius, Color c) const;
    // ––––––––[ ring ] strokes a rounded rectangle outline
    void ring(Rect r, float radius, Color c, float width = 1.5f) const;
    // ––––––––[ text ] draws left-aligned text inside bounds
    void text(std::string_view s, Color c, float size = 0.0f) const;
    // ––––––––[ vg ] raw context escape hatch for advanced painting
    NVGcontext* vg() const noexcept { return vg_; }
    // ––––––––[ bounds ] widget bounds for this frame
    Rect bounds() const noexcept { return bounds_; }
    // ––––––––[ theme ] active theme for this frame
    const Theme& theme() const noexcept { return theme_; }

private:
    NVGcontext* vg_;
    const Theme& theme_;
    Rect bounds_;
};

// ––––––––[ SimpleWidget ] override paint() and onClick(), ignore the rest
class SimpleWidget : public Widget {
public:
    // ––––––––[ SimpleWidget ] default bounds, layouts assign geometry
    explicit SimpleWidget(Rect bounds = Rect{}) : Widget(bounds) {}
    // ––––––––[ paint ] draws widget state, called every frame
    virtual void paint(const Painter& p) = 0;
    // ––––––––[ onPress ] return true when a mouse press is consumed
    virtual bool onPress(float x, float y);
    // ––––––––[ preferredWidth ] size hint used when autoSize is enabled
    virtual float preferredWidth(const Theme& theme, const Window& w) const;
    // ––––––––[ preferredHeight ] height hint used when autoSize is enabled
    virtual float preferredHeight(const Theme& theme, const Window& w) const;
    void draw(NVGcontext* vg, const Theme& theme, const Window& w) override;
    EventResult handleEvent(const Event& e, const Window& w) override;
    float naturalWidth(const Theme& theme, const Window& w) const override;
    float naturalHeight(const Theme& theme, const Window& w) const override;
};

} // namespace fluffy
