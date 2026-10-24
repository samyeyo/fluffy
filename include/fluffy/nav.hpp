// include/fluffy/nav.hpp

// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
// Nav — hierarchical navigation control (Fluent UI Nav style).
//
// MIT License - Copyright (c) 2026 - Samir Tine
// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

#pragma once
#include <fluffy/animation.hpp>
#include <fluffy/svg_icon.hpp>
#include <fluffy/widget.hpp>
#include <functional>
#include <string>
#include <vector>
namespace fluffy {
using Icons::SvgIconRef;

// ––––––––[ Nav ] vertical navigation with expandable groups and selectable items.
class Nav final : public Widget {
public:
    // ––––––––[ NavItem ] a single selectable item with optional icon and click callback.
    struct NavItem {
        SvgIconRef            icon{nullptr, 0};
        std::string           label;
        std::string           id;
        bool                  enabled = true;
        std::function<void()> on_click;
    };

    // ––––––––[ NavGroup ] an expandable category containing NavItems, with smooth expand animation.
    struct NavGroup {
        std::string          label;
        std::string          id;
        SvgIconRef           icon{nullptr, 0};
        bool                 expanded = true;
        bool                 enabled   = true;
        std::vector<NavItem> items;
        mutable SmoothAnim   expandAnim{0.0f, 0.0f};  // 0=collapsed, 1=expanded
    };

    Nav() = default;
    Nav(Rect bounds);

    void addGroup(NavGroup group);
    void addItem(NavItem item);

    // ––––––––[ expandGroup ] expands a group by id (triggers animation).
    void expandGroup(const std::string& id);
    // ––––––––[ collapseGroup ] collapses a group by id (triggers animation).
    void collapseGroup(const std::string& id);
    // ––––––––[ expandAll ] expands all groups.
    void expandAll();
    // ––––––––[ collapseAll ] collapses all groups.
    void collapseAll();

    void         draw(NVGcontext* vg, const Theme& theme, const Window& w) override;
    EventResult  handleEvent(const Event& e, const Window& w) override;
    void         updateHover(float mx, float my) noexcept override;
    void         tick(float dt, const Theme& theme) noexcept override;
    void         setProperty(std::string_view name, const YamlNode& value) override;
    float        naturalWidth(const Theme& theme, const Window& w) const override;
    float        naturalHeight(const Theme& theme, const Window& w) const override;

    // ––––––––[ setSelectedItem ] sets the selected item by id and fires on_select.
    void setSelectedItem(const std::string& id);
    const std::string& selectedItemId() const noexcept { return selectedId_; }

    bool isFocusable() const noexcept override { return true; }
    void setFocused(bool f) noexcept override { focused_ = f; }
    bool isFocused() const noexcept { return focused_; }

    using SelectHandler = std::function<void(const std::string&)>;
    void setOnSelect(SelectHandler h) { on_select_ = std::move(h); }
    // ––––––––[ onSelect ] chainable selection handler, preferred over setOnSelect
    Nav& onSelect(SelectHandler h) { on_select_ = std::move(h); return *this; }

    // ––––––––[ computeMinWidth ] calculates the minimum width from first-level items (group headers + root items).
    void computeMinWidth(const Theme& theme, const Window& w);
    void setMinWidth(float v) noexcept { minWidth_ = v; }
    float minWidth() const noexcept { return minWidth_; }

    using WidthChangeHandler = std::function<void(float)>;
    void setOnWidthChange(WidthChangeHandler h) { on_width_change_ = std::move(h); }

private:
    // ––––––––[ RenderedLine ] flattened representation of a visible row.
    struct RenderedLine {
        std::string  id;
        std::string  label;
        SvgIconRef   icon{nullptr, 0};
        float        y{0.0f};
        float        h{0.0f};
        int          indent{0};
        bool         isItem{false};
        bool         enabled{true};
        std::size_t  groupIdx{0};
        std::size_t  itemIdx{0};
        bool         isGroupHeader{false};
        bool         groupExpanded{false};
        std::function<void()> on_click;
    };

    // ––––––––[ rebuildLines ] flattens the tree into a RenderedLine array.
    void rebuildLines(const Theme& theme) const;

    // ––––––––[ rowHeight ] uniform row height based on theme metrics.
    float rowHeight(const Theme& theme) const noexcept;
    // ––––––––[ indentPx ] pixel width of one indent level.
    float indentPx(const Theme& theme) const noexcept;

    std::vector<NavGroup>    groups_;
    std::vector<NavItem>     rootItems_;
    std::string              selectedId_;

    mutable std::vector<RenderedLine> lines_;

    bool          focused_    = false;
    int           hoveredIdx_ = -1;
    SmoothAnim    hoverAnim_;
    SmoothAnim    scrollAnim_;  // fade scrollbar on hover
    SelectHandler      on_select_;
    WidthChangeHandler on_width_change_;
    float         minWidth_ = 80.0f;
    bool          resizing_ = false;
    float         resizeStartX_ = 0.0f;
    float         resizeOrigW_  = 0.0f;
    bool          nearRightEdge_ = false;

    // ––––––––[ scrollY_ ] vertical scroll offset in logical pixels.
    float         scrollY_            = 0.0f;
    bool          scrollDragging_     = false;
    float         scrollDragStartY_   = 0.0f;
    float         scrollDragOrigY_    = 0.0f;
    bool          scrollbarHovered_   = false;
    bool          scrollWheelActive_  = false;
    float         scrollWheelTimer_   = 0.0f;
};

} // namespace fluffy
