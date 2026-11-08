// include/fluffy/menu.hpp

// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
// Menu — horizontal or vertical menu widget with items (icon + label + shortcut).
//
// MIT License - Copyright (c) 2026 - Samir Tine
// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

#pragma once
#include <fluffy/animation.hpp>
#include <fluffy/inline.hpp>
#include <fluffy/svg_icon.hpp>
#include <fluffy/widget.hpp>
#include <functional>
#include <memory>
#include <string>
#include <vector>
namespace fluffy {
using Icons::SvgIconRef;

// ––––––––[ Menu ] orientable list of items with hover highlights and click callbacks.
class Menu final : public Widget {
public:
    enum class Orientation { Horizontal, Vertical };

    // ––––––––[ Item ] a single menu entry with optional icon, label, shortcut, and click handler.
    struct Item {
        SvgIconRef               icon{nullptr, 0};
        std::string              label;
        Inline                   inlineLabel;
        std::string              shortcut;
        std::function<void()>    on_click;
        bool                     enabled   = true;
        bool                     separator = false;
        std::unique_ptr<Menu>    submenu;  // dropdown submenu (for horizontal menu bar items)
    };

    Menu(Rect bounds = Rect{}, Orientation orient = Orientation::Vertical);

    void         draw(NVGcontext* vg, const Theme& theme, const Window& w) override;
    EventResult  handleEvent(const Event& e, const Window& w) override;
    void         updateHover(float mx, float my) noexcept override;
    void         tick(float dt, const Theme& theme) noexcept override;
    void         setProperty(std::string_view name, const YamlNode& value) override;
    float        naturalWidth(const Theme& theme, const Window& w) const override;
    float        naturalHeight(const Theme& theme, const Window& w) const override;

    // ––––––––[ makePopup ] deep-copies items with closePopup-wrapped callbacks (recursively handles submenus).
    static std::unique_ptr<Menu> makePopup(const std::vector<Item>& items, Window& w);

    void addItem(Item item) { items_.push_back(std::move(item)); }
    void clearItems()       { items_.clear(); hovered_index_ = -1; }
    const std::vector<Item>& items() const noexcept { return items_; }

    Orientation orientation() const noexcept { return orientation_; }
    void setOrientation(Orientation o) noexcept { orientation_ = o; }

    bool  isFocusable() const noexcept override { return true; }
    void  setFocused(bool f) noexcept override { focused_ = f; }
    bool  isFocused() const noexcept { return focused_; }
    bool isMenu() const noexcept override { return true; }

private:
    // ––––––––[ itemHeight ] uniform row height based on theme metrics.
    float itemHeight(const Theme& theme) const noexcept;
    // ––––––––[ itemAt ] returns index of item at (mx, my) or -1 if none. Uses Window for text measurement in horizontal mode.
    int   itemAt(float mx, float my, const Theme& theme, const Window& w) const noexcept;

    bool              focused_        = false;
    Orientation       orientation_    = Orientation::Vertical;
    std::vector<Item> items_;
    int               hovered_index_  = -1;
    SmoothAnim        hoverAnim_;
};

} // namespace fluffy
