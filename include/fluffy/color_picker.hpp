// include/fluffy/color_picker.hpp
// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
// ColorPicker — HSV bars + editable RGBA/hex fields inside a rounded card.
//
// MIT License - Copyright (c) 2026 - Samir Tine
// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

#pragma once
#include <fluffy/animation.hpp>
#include <fluffy/widget.hpp>
#include <functional>
#include <string>
namespace fluffy {

// ––––––––[ ColorPicker ] HSV hue/sat/val bars + RGBA text entry + hex field.
class ColorPicker final : public Widget {
public:
    using ChangeHandler = std::function<void(Color)>;

    ColorPicker(Rect bounds = Rect{}, Color initial = Color::fromHex(0xFFFFFFFF));

    void         draw(NVGcontext* vg, const Theme& theme, const Window& w) override;
    EventResult  handleEvent(const Event& e, const Window& w) override;
    void         updateHover(float mx, float my) noexcept override;
    void         tick(float dt, const Theme& theme) noexcept override;
    void         setProperty(std::string_view name, const YamlNode& value) override;
    float        naturalWidth(const Theme& theme, const Window& w) const override;
    float        naturalHeight(const Theme& theme, const Window& w) const override;

    Color color() const noexcept { return color_; }
    void  setColor(Color c) noexcept;
    void  setOnChange(ChangeHandler h) { on_change_ = std::move(h); }
    // ––––––––[ onChange ] chainable change handler, preferred over setOnChange
    ColorPicker& onChange(ChangeHandler h) { on_change_ = std::move(h); return *this; }
    bool  isFocusable() const noexcept override { return true; }
    void  setFocused(bool f) noexcept override;
    bool  isFocused() const noexcept { return focused_; }

private:
    Color         color_ = Color::fromHex(0xFFFFFFFF);
    float         hue_   = 0.0f;
    float         sat_   = 0.0f;
    float         val_   = 1.0f;
    char          hex_buf_[10] = "FFFFFFFF";

    int           drag_bar_   = -1;   // 0=H,1=S,2=V,3=A, -1=none
    int           edit_field_ = -1;   // -1=none, 0=R,1=G,2=B,3=A,4=Hex
    std::string   edit_buf_;
    bool          hovered_ = false;
    bool          focused_ = false;
    float         cursor_t_ = 0.0f;   // cursor blink timer
    SmoothAnim    hoverAnim_;
    ChangeHandler on_change_;

    void updateHexFromColor() noexcept;
    void syncHSV() noexcept;
    void commitEdit() noexcept;

    // ––––––––[ fieldRect ] hit-test rectangle for an RGBA or hex field.
    Rect fieldRect(int field) const noexcept;
};

} // namespace fluffy
