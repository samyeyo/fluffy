// include/fluffy/layout.hpp
// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
// Layout — positions Container children (Horizontal / Vertical / Grid).
//
// MIT License - Copyright (c) 2026 - Samir Tine
// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

#pragma once
#include <fluffy/geom.hpp>
#include <fluffy/widget.hpp>
#include <memory>
#include <string>
#include <string_view>
#include <vector>
namespace fluffy {
class YamlNode;
enum class Align {
    Start,
    Center,
    End,
};
enum class Justify {
    Start,
    Center,
    End,
    SpaceBetween,
    SpaceEvenly,
};
struct Theme;
class Layout {
public:
    virtual ~Layout() = default;
    virtual void apply(const Rect& containerBounds,
                       std::vector<std::unique_ptr<Widget>>& children,
                       const Theme& theme) = 0;
    virtual void setProperty(std::string_view name, const YamlNode& value);
    virtual bool isHorizontal() const noexcept { return false; }
    virtual bool isVertical() const noexcept { return false; }
    Layout& setSpacing(float s) noexcept { spacing_ = s; return *this; }
    Layout& setPadding(float p) noexcept { padding_ = p; return *this; }
    Layout& setMargin(float m) noexcept { margin_ = m; return *this; }
    Layout& align(Align a) noexcept { align_ = a; return *this; }
    Layout& justify(Justify j) noexcept { justify_ = j; return *this; }
    Align   align()   const noexcept { return align_; }
    Justify justify() const noexcept { return justify_; }
    float spacing() const noexcept { return spacing_; }
    float padding() const noexcept { return padding_; }
    float margin()  const noexcept { return margin_; }
protected:
    float   spacing_  = 0.0f;
    float   padding_ = 0.0f;
    float   margin_  = 0.0f;
    Align   align_   = Align::Start;
    Justify justify_ = Justify::Start;
    float crossOffset(float slot_size, float child_size) const noexcept;
};
class HorizontalLayout final : public Layout {
public:
    void apply(const Rect& containerBounds,
               std::vector<std::unique_ptr<Widget>>& children,
               const Theme& theme) override;
    bool isHorizontal() const noexcept override { return true; }
};
class VerticalLayout final : public Layout {
public:
    void apply(const Rect& containerBounds,
               std::vector<std::unique_ptr<Widget>>& children,
               const Theme& theme) override;
    bool isVertical() const noexcept override { return true; }
};
class GridLayout final : public Layout {
public:
    explicit GridLayout(int cols = 2) : cols_(cols) {}
    void apply(const Rect& containerBounds,
               std::vector<std::unique_ptr<Widget>>& children,
               const Theme& theme) override;
    GridLayout& setCols(int c) noexcept { cols_ = std::max(1, c); return *this; }
    GridLayout& setRows(int r) noexcept { rows_ = std::max(0, r); return *this; }
    int cols() const noexcept { return cols_; }
    int rows() const noexcept { return rows_; }
    void setProperty(std::string_view name, const YamlNode& value) override;
private:
    // ––––––––[ cols_ ] number of columns
    int cols_ = 2;
    // ––––––––[ rows_ ] 0 = auto
    int rows_ = 0;
};
// ––––––––[ vertical ] one-line vertical layout with spacing and padding
inline std::unique_ptr<VerticalLayout> vertical(float spacing = 8.0f, float padding = 0.0f) {
    auto l = std::make_unique<VerticalLayout>();
    l->setSpacing(spacing);
    l->setPadding(padding);
    return l;
}
// ––––––––[ horizontal ] one-line horizontal layout with spacing and padding
inline std::unique_ptr<HorizontalLayout> horizontal(float spacing = 8.0f, float padding = 0.0f) {
    auto l = std::make_unique<HorizontalLayout>();
    l->setSpacing(spacing);
    l->setPadding(padding);
    return l;
}
// ––––––––[ grid ] one-line grid layout with column count
inline std::unique_ptr<GridLayout> grid(int cols = 2, float spacing = 8.0f, float padding = 0.0f) {
    auto l = std::make_unique<GridLayout>(cols);
    l->setSpacing(spacing);
    l->setPadding(padding);
    return l;
}
}  
