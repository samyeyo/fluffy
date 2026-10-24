// include/fluffy/constraint.hpp
// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
// Constraint — anchor-based per-widget constraints, small linear solver.
// Anchor model: left/right/top/bottom/width/height/centerX/centerY plus parent/prev.
//
// MIT License - Copyright (c) 2026 - Samir Tine
// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

#pragma once
#include <fluffy/geom.hpp>
#include <map>
#include <string>
#include <string_view>

namespace fluffy {
class Widget;
class YamlNode;

// ––––––––[ Anchor ] left/right/top/bottom/width/height/centerX/centerY
enum class Anchor {
    Left,
    Right,
    Top,
    Bottom,
    Leading,
    Trailing,
    Width,
    Height,
    CenterX,
    CenterY,
    Baseline,
    None
};

// ––––––––[ Rel ] relation for inequality handling
enum class Rel {
    Equal,
    GreaterEqual,
    LessEqual
};

// ––––––––[ Constraint ] one anchor constraint
struct Constraint {
    std::string targetName; // "parent", "prev", or "" for constant
    Anchor targetAnchor = Anchor::None;
    float multiplier = 1.0f;
    float constant = 0.0f;
    float priority = 1000.0f;
    Rel rel = Rel::Equal;
    // For C++ direct widget pointer (resolved)
    Widget* targetWidget = nullptr;
};

// ––––––––[ ConstraintSet ] map from anchor -> constraint
struct ConstraintSet {
    std::map<Anchor, Constraint> entries;

    bool empty() const noexcept { return entries.empty(); }
    bool has(Anchor a) const noexcept { return entries.find(a) != entries.end(); }
    void clear() noexcept { entries.clear(); }

    // ––––––––[ fluent helpers ] C++ API: constrain(*widget).leftToParent(16)
    ConstraintSet& left(Anchor targetA, Widget* target, float mult = 1.0f, float c = 0.0f, float prio = 1000.0f);
    ConstraintSet& right(Anchor targetA, Widget* target, float mult = 1.0f, float c = 0.0f, float prio = 1000.0f);
    ConstraintSet& top(Anchor targetA, Widget* target, float mult = 1.0f, float c = 0.0f, float prio = 1000.0f);
    ConstraintSet& bottom(Anchor targetA, Widget* target, float mult = 1.0f, float c = 0.0f, float prio = 1000.0f);
    ConstraintSet& width(Anchor targetA, Widget* target, float mult = 1.0f, float c = 0.0f, float prio = 1000.0f);
    ConstraintSet& height(Anchor targetA, Widget* target, float mult = 1.0f, float c = 0.0f, float prio = 1000.0f);
    ConstraintSet& centerX(Anchor targetA, Widget* target, float mult = 1.0f, float c = 0.0f, float prio = 1000.0f);
    ConstraintSet& centerY(Anchor targetA, Widget* target, float mult = 1.0f, float c = 0.0f, float prio = 1000.0f);

    // ––––––––[ sugars ] parent-relative
    ConstraintSet& leftToParent(float c = 0.0f) { return left(Anchor::Left, nullptr, 1.0f, c); }
    ConstraintSet& rightToParent(float c = 0.0f) { return right(Anchor::Right, nullptr, 1.0f, c); }
    ConstraintSet& topToParent(float c = 0.0f) { return top(Anchor::Top, nullptr, 1.0f, c); }
    ConstraintSet& bottomToParent(float c = 0.0f) { return bottom(Anchor::Bottom, nullptr, 1.0f, c); }
    ConstraintSet& centerXToParent(float c = 0.0f) { return centerX(Anchor::CenterX, nullptr, 1.0f, c); }
    ConstraintSet& centerYToParent(float c = 0.0f) { return centerY(Anchor::CenterY, nullptr, 1.0f, c); }
    ConstraintSet& widthPercentOfParent(float percent, float c = 0.0f) { return width(Anchor::Width, nullptr, percent, c); }
    ConstraintSet& heightPercentOfParent(float percent, float c = 0.0f) { return height(Anchor::Height, nullptr, percent, c); }

    // ––––––––[ constant ] width: 120
    ConstraintSet& widthConstant(float w) { entries[Anchor::Width] = Constraint{"", Anchor::None, 0.0f, w, 1000.0f, Rel::Equal, nullptr}; return *this; }
    ConstraintSet& heightConstant(float h) { entries[Anchor::Height] = Constraint{"", Anchor::None, 0.0f, h, 1000.0f, Rel::Equal, nullptr}; return *this; }
};

// ––––––––[ helpers ] parsing and anchor name conversion
Anchor anchorFromString(std::string_view s) noexcept;
std::string anchorToString(Anchor a);
bool parseConstraintString(std::string_view raw, Constraint& out) noexcept;
bool parseConstraintsNode(const YamlNode& node, ConstraintSet& out) noexcept;

// ––––––––[ constrain ] helper to get/set constraints on a widget
ConstraintSet& constrain(Widget& w) noexcept;
const ConstraintSet* constraintsOf(const Widget& w) noexcept;
void clearConstraints(Widget& w) noexcept;

} // namespace fluffy
