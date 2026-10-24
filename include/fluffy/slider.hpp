// include/fluffy/slider.hpp

// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

// Slider — horizontal draggable track with thumb and animated hover.

#pragma once
#include <fluffy/animation.hpp>
#include <fluffy/widget.hpp>
#include <functional>
namespace fluffy {

// ––––––––[ Slider ] horizontal slider with track, thumb, and continuous value range.
class Slider final : public Widget {
public:
    using ChangeHandler = std::function<void(float)>;

    Slider(Rect bounds = Rect{}, float initial = 0.0f);

    void         draw(NVGcontext* vg, const Theme& theme, const Window& w) override;
    EventResult  handleEvent(const Event& e, const Window& w) override;
    void         updateHover(float mx, float my) noexcept override;
    void         tick(float dt, const Theme& theme) noexcept override;
    void         setProperty(std::string_view name, const YamlNode& value) override;
    float        naturalWidth(const Theme& theme, const Window& w) const override;
    float        naturalHeight(const Theme& theme, const Window& w) const override;

    float value() const noexcept { return value_; }
    void  setValue(float v) noexcept;
    float minValue() const noexcept { return min_; }
    float maxValue() const noexcept { return max_; }
    void  setRange(float minVal, float maxVal) noexcept;
    bool  isFocusable() const noexcept override { return true; }
    void  setFocused(bool f) noexcept override { focused_ = f; }
    bool  isFocused() const noexcept { return focused_; }
    void  setOnChange(ChangeHandler h) { on_change_ = std::move(h); }
    // ––––––––[ onChange ] chainable change handler, preferred over setOnChange
    Slider& onChange(ChangeHandler h) { on_change_ = std::move(h); return *this; }
    // ––––––––[ setDecimals ] formats value with N decimal places
    void setDecimals(int n) { value_format_ = "{:." + std::to_string(n < 0 ? 0 : n) + "f}"; }
    bool  showValue() const noexcept { return show_value_; }
    void  setShowValue(bool v) noexcept { show_value_ = v; }
    const std::string& valueFormat() const noexcept { return value_format_; }
    void  setValueFormat(std::string fmt) noexcept { value_format_ = std::move(fmt); }

private:
    float         value_      = 0.0f;
    float         min_        = 0.0f;
    float         max_        = 1.0f;
    bool          show_value_ = false;
    bool          hovered_    = false;
    bool          dragging_   = false;
    bool          focused_    = false;
    std::string   value_format_ = "{:.2f}";
    SmoothAnim    hoverAnim_;
    ChangeHandler on_change_;
};

} // namespace fluffy
