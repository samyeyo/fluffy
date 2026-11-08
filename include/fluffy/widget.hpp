// include/fluffy/widget.hpp

// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

// Widget — abstract base for every UI element (draw / handleEvent / geometry).

#pragma once
#include <fluffy/event.hpp>
#include <fluffy/geom.hpp>
#include <fluffy/theme.hpp>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
struct NVGcontext;
namespace fluffy {
class YamlNode;
class Window;
enum class EventResult { Ignored, Consumed };
enum class AutoSize {
  Disabled,
  Width,
  Height,
  Both
};
class Widget {
public:
    Widget()                          = default;
    explicit Widget(Rect bounds)    : bounds_(bounds) {}
    virtual ~Widget();
    Widget(const Widget&)             = delete;
    Widget& operator=(const Widget&)  = delete;
    Rect  bounds() const noexcept { return bounds_; }
    void  setBounds(Rect r) noexcept { bounds_ = r; window_sized_ = false; }
    void  fillToWindow(float w, float h) noexcept {
        bounds_ = Rect(0.0f, 0.0f, w, h);
        window_sized_ = true;
    }
    bool  windowSized() const noexcept { return window_sized_; }
    float width()  const noexcept { return bounds_.width();  }
    float height() const noexcept { return bounds_.height(); }
    bool  visible() const noexcept { return visible_; }
    void  setVisible(bool v) noexcept { visible_ = v; }
    bool  enabled() const noexcept { return enabled_; }
    void  setEnabled(bool e) noexcept { enabled_ = e; }
    bool  containsPoint(float x, float y) const noexcept {
        return bounds_.containsPoint(x, y);
    }
    const std::string& font() const noexcept { return font_; }
    Widget& setFont(std::string f) noexcept { font_ = std::move(f); return *this; }
    float textSize() const noexcept { return textSize_; }
    Widget& setTextSize(float size) noexcept { textSize_ = size; return *this; }
    AutoSize autoSize() const noexcept { return auto_size_; }
    void     setAutoSize(AutoSize a) noexcept { auto_size_ = a; }
    virtual float naturalWidth(const Theme& theme, const Window& w) const;
    virtual float naturalHeight(const Theme& theme, const Window& w) const;
    std::string&       userdata()       noexcept { return userdata_; }
    const std::string& userdata() const noexcept { return userdata_; }
    float stretchWeight() const noexcept { return stretch_weight_; }
    void setStretchWeight(float w) noexcept { stretch_weight_ = w; }
    Window* window() const noexcept { return window_; }
    virtual void setWindow(Window* w) noexcept { window_ = w; }
    virtual void setProperty(std::string_view name, const YamlNode& value);
    virtual void setEvent(std::string_view name, std::string_view handlerName);
    virtual void triggerEvent(
        std::string_view eventName,
        const std::vector<std::string>& args = {}
    ) const;
    virtual void        draw(NVGcontext* vg, const Theme& theme, const Window& w) = 0;
    virtual EventResult handleEvent(const Event& e, const Window& w)  = 0;
    virtual void        onResize(const ResizeEvent& e) { (void)e; }
    virtual bool        isFocusable() const noexcept { return false; }
    virtual void        setFocused(bool /*f*/) noexcept {}
    virtual void        clearFocus() noexcept { setFocused(false); }
    virtual void        updateHover(float /*mx*/, float /*my*/) noexcept {}
    virtual void        tick(float /*dt*/, const Theme& /*theme*/) noexcept {}
    virtual bool        fillsParent() const noexcept { return false; }
    virtual bool        isContainer() const noexcept { return false; }
    virtual bool        isMenu() const noexcept { return false; }
    virtual void        forEachChild(const std::function<void(Widget*)>& fn) { (void)fn; }
    virtual bool        isTooltip() const noexcept { return false; }

    // ––––––––[ accentColor ] returns widget's override if set, else theme's global accent.
    Color accentColor(const Theme& theme) const noexcept {
        return accent_override_.value_or(theme.colors.accent);
    }
    // ––––––––[ accentPressedColor ] returns widget's override x2 or theme's accent_pressed.
    Color accentPressedColor(const Theme& theme) const noexcept {
        return accent_pressed_override_.value_or(theme.colors.accent_pressed);
    }
    // ––––––––[ setAccentOverride ] set per-widget accent color (overrides theme).
    void setAccentOverride(Color c) noexcept { accent_override_ = c; }
    void setAccentPressedOverride(Color c) noexcept { accent_pressed_override_ = c; }
    void clearAccentOverride() noexcept { accent_override_.reset(); accent_pressed_override_.reset(); }

protected:
    bool        enabled_  = true;
    bool        visible_  = true;
    bool        window_sized_ = false;
    Rect        bounds_{};
    std::string font_;
    float       textSize_ = 0.0f;
    AutoSize    auto_size_ = AutoSize::Both;
    std::string userdata_;
    Window*     window_ = nullptr;
    std::vector<std::pair<std::string, std::string>> event_bindings_;
    float stretch_weight_ = 1.0f;
    std::optional<Color> accent_override_;
    std::optional<Color> accent_pressed_override_;
};
}  // namespace fluffy
