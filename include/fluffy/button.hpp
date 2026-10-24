// include/fluffy/button.hpp

// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

// Button — rounded clickable surface with animated hover/press and label.

#pragma once
#include <fluffy/animation.hpp>
#include <fluffy/inline.hpp>
#include <fluffy/menu.hpp>
#include <fluffy/widget.hpp>
#include <functional>
#include <string>
#include <vector>
namespace fluffy {
class Button final : public Widget {
public:
    using ClickHandler = std::function<void()>;
    explicit Button(std::string label) : Button(Rect{}, std::move(label)) {}
    explicit Button(Inline  content)   : Button(Rect{}, std::move(content)) {}
    Button(Rect bounds = Rect{}, std::string label = {});
    Button(Rect bounds, Inline content);
    void         draw(NVGcontext* vg, const Theme& theme, const Window& w)  override;
    EventResult  handleEvent(const Event& e, const Window& w) override;
    void         updateHover(float mx, float my) noexcept override;
    void         tick(float dt, const Theme& theme) noexcept override;
    void         setProperty(std::string_view name, const YamlNode& value) override;
    float naturalWidth(const Theme& theme, const Window& w) const override;
    float naturalHeight(const Theme& theme, const Window& w) const override;
    const std::string& label() const noexcept { return label_; }
    void               setLabel(std::string l);
    void               setInline(Inline c);
    // ––––––––[ setText ] unified text setter accepts plain strings and inline content
    Button& setText(Inline c) { setInline(std::move(c)); return *this; }
    void   setOnClick(ClickHandler h) { on_click_ = std::move(h); }
    // ––––––––[ onClick ] chainable click handler, preferred over setOnClick
    Button& onClick(ClickHandler h) { on_click_ = std::move(h); return *this; }
    void setDropdownMenu(std::unique_ptr<Menu> menu) { dropdownMenu_ = std::move(menu); }
    Menu* dropdownMenu() const noexcept { return dropdownMenu_.get(); }
    bool hasDropdownMenu() const noexcept { return dropdownMenu_ != nullptr; }
    bool isHovered() const noexcept { return hovered_; }
    bool isPressed() const noexcept { return pressed_; }
    bool isFocusable() const noexcept override { return true; }
    void setFocused(bool f) noexcept override { focused_ = f; }
    bool isFocused() const noexcept { return focused_; }
private:
    std::string                 label_;   
    std::vector<Inline::Run>   inline_;  
    bool                        is_inline_ = false;
    ClickHandler on_click_;
    bool         hovered_ = false;
    bool         pressed_ = false;
    bool         focused_ = false;
    std::unique_ptr<Menu> dropdownMenu_;
    SmoothAnim   hoverAnim_;
    SmoothAnim   pressAnim_;
};
}  

