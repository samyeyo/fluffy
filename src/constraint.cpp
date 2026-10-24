// src/constraint.cpp
// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
// Constraint — anchor parsing, ConstraintSet helpers, global storage.
//
// MIT License - Copyright (c) 2026 - Samir Tine
// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

#include <fluffy/constraint.hpp>
#include <fluffy/widget.hpp>
#include <fluffy/yaml_fwd.hpp>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <string>
#include <unordered_map>

namespace fluffy {
namespace {
std::unordered_map<Widget*, ConstraintSet> g_constraints;

// ––––––––[ toLowerChar ] ctype wrappers without throwing casts
char toLowerChar(char c) noexcept {
    return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
}

bool isSpaceChar(char c) noexcept {
    return std::isspace(static_cast<unsigned char>(c)) != 0;
}

bool isDigitChar(char c) noexcept {
    return std::isdigit(static_cast<unsigned char>(c)) != 0;
}

// ––––––––[ trimView ] whitespace trim, never throws
std::string trimStr(std::string_view s) noexcept {
    size_t a = 0;
    while (a < s.size() && isSpaceChar(s[a])) ++a;
    size_t b = s.size();
    while (b > a && isSpaceChar(s[b - 1])) --b;
    return std::string(s.substr(a, b - a));
}

// ––––––––[ parseFloatNoThrow ] strtof wrapper, returns 0 on failure
float parseFloatNoThrow(const std::string& s) noexcept {
    if (s.empty()) return 0.0f;
    const char* begin = s.c_str();
    char* end = nullptr;
    float v = std::strtof(begin, &end);
    if (end == begin) return 0.0f;
    return v;
}
} // namespace

// ––––––––[ anchorFromString ] case-insensitive
Anchor anchorFromString(std::string_view s) noexcept {
    std::string low;
    low.reserve(s.size());
    for (char c : s) low.push_back(toLowerChar(c));
    if (low == "left" || low == "leading") return Anchor::Left;
    if (low == "right" || low == "trailing") return Anchor::Right;
    if (low == "top") return Anchor::Top;
    if (low == "bottom") return Anchor::Bottom;
    if (low == "width") return Anchor::Width;
    if (low == "height") return Anchor::Height;
    if (low == "centerx" || low == "center_x" || low == "center-x") return Anchor::CenterX;
    if (low == "centery" || low == "center_y" || low == "center-y") return Anchor::CenterY;
    if (low == "baseline") return Anchor::Baseline;
    return Anchor::None;
}

std::string anchorToString(Anchor a) {
    switch (a) {
    case Anchor::Left: return "left";
    case Anchor::Right: return "right";
    case Anchor::Top: return "top";
    case Anchor::Bottom: return "bottom";
    case Anchor::Leading: return "left";
    case Anchor::Trailing: return "right";
    case Anchor::Width: return "width";
    case Anchor::Height: return "height";
    case Anchor::CenterX: return "centerX";
    case Anchor::CenterY: return "centerY";
    case Anchor::Baseline: return "baseline";
    default: return "none";
    }
}

// ––––––––[ parseConstraintString ] "parent.left+40", "prev.bottom-16", "120", "parent.width*0.5"
bool parseConstraintString(std::string_view raw, Constraint& out) noexcept {
    out = Constraint{};
    std::string s = trimStr(raw);
    if (s.empty()) return false;

    // Check for pure numeric constant like "120" or "120.5"
    bool isNum = true;
    bool hasDot = false;
    size_t start = 0;
    if (s[0] == '+' || s[0] == '-') start = 1;
    for (size_t i = start; i < s.size(); ++i) {
        if (isDigitChar(s[i])) continue;
        if (s[i] == '.' && !hasDot) { hasDot = true; continue; }
        isNum = false;
        break;
    }
    if (isNum && s.size() > start) {
        out.targetName = "";
        out.targetAnchor = Anchor::None;
        out.multiplier = 0.0f;
        out.constant = parseFloatNoThrow(s);
        return true;
    }
    if (isNum) return false;

    // Parse target.anchor [ * mult ] [ +/- const ]
    // Find '.' separating target and anchor
    size_t dot = s.find('.');
    if (dot == std::string::npos) {
        return false;
    }
    std::string target = trimStr(std::string_view(s).substr(0, dot));
    std::string rest = trimStr(std::string_view(s).substr(dot + 1));

    // Find * and +/- in rest
    size_t star = rest.find('*');
    size_t plus = rest.find('+', 1);
    size_t minus = rest.find('-', 1);
    // Note: search +/- from 1 to skip a leading sign already handled.
    // Actually anchor names never contain +/-, so search from 0 except
    // a leading sign belongs to nothing here; rest never starts with sign
    // because anchor comes first. Search whole string for robustness:
    plus = rest.find('+');
    minus = rest.find('-');

    size_t anchorEnd = rest.size();
    if (star != std::string::npos) anchorEnd = std::min(anchorEnd, star);
    if (plus != std::string::npos) anchorEnd = std::min(anchorEnd, plus);
    if (minus != std::string::npos) anchorEnd = std::min(anchorEnd, minus);

    std::string anchorStr = trimStr(std::string_view(rest).substr(0, anchorEnd));
    out.targetAnchor = anchorFromString(anchorStr);
    if (out.targetAnchor == Anchor::None) return false;
    out.targetName = target;

    // Multiplier part
    float mult = 1.0f;
    if (star != std::string::npos) {
        size_t multStart = star + 1;
        size_t multEnd = rest.size();
        if (plus != std::string::npos && plus > star) multEnd = std::min(multEnd, plus);
        if (minus != std::string::npos && minus > star) multEnd = std::min(multEnd, minus);
        std::string multStr = trimStr(std::string_view(rest).substr(multStart, multEnd - multStart));
        if (!multStr.empty()) mult = parseFloatNoThrow(multStr);
    }

    // Constant part
    float cons = 0.0f;
    size_t constPos = std::string::npos;
    if (plus != std::string::npos) constPos = plus;
    else if (minus != std::string::npos) constPos = minus;
    if (constPos != std::string::npos) {
        std::string constStr = trimStr(std::string_view(rest).substr(constPos));
        if (!constStr.empty()) cons = parseFloatNoThrow(constStr);
    }

    out.multiplier = mult;
    out.constant = cons;
    return true;
}

bool parseConstraintsNode(const YamlNode& node, ConstraintSet& out) noexcept {
    if (!node.IsMap()) return false;
    out.clear();
    for (const auto& child : node) {
        std::string key = child.key();
        Anchor a = anchorFromString(key);
        if (a == Anchor::None) continue;
        std::string raw = child.asString();
        Constraint c;
        if (raw.empty()) {
            // Numeric value written without quotes: use asFloat constant
            float v = child.asFloat();
            c.targetName = "";
            c.targetAnchor = Anchor::None;
            c.multiplier = 0.0f;
            c.constant = v;
        } else {
            if (!parseConstraintString(raw, c)) continue;
        }
        out.entries[a] = c;
    }
    return !out.empty();
}

// ––––––––[ setEntry ] shared helper for fluent setters
static ConstraintSet& setEntry(ConstraintSet& cs, Anchor self,
                               Anchor ta, Widget* t,
                               float m, float c, float p) noexcept {
    Constraint e;
    e.targetAnchor = ta;
    e.multiplier = m;
    e.constant = c;
    e.priority = p;
    e.rel = Rel::Equal;
    e.targetWidget = t;
    if (t == nullptr) {
        // nullptr target means parent for positional anchors
        if (ta == Anchor::None) e.targetName = "";
        else e.targetName = "parent";
    } else {
        e.targetName = "";
    }
    cs.entries[self] = e;
    return cs;
}

// ––––––––[ ConstraintSet fluent ] helpers
ConstraintSet& ConstraintSet::left(Anchor ta, Widget* t, float m, float c, float p) {
    return setEntry(*this, Anchor::Left, ta, t, m, c, p);
}
ConstraintSet& ConstraintSet::right(Anchor ta, Widget* t, float m, float c, float p) {
    return setEntry(*this, Anchor::Right, ta, t, m, c, p);
}
ConstraintSet& ConstraintSet::top(Anchor ta, Widget* t, float m, float c, float p) {
    return setEntry(*this, Anchor::Top, ta, t, m, c, p);
}
ConstraintSet& ConstraintSet::bottom(Anchor ta, Widget* t, float m, float c, float p) {
    return setEntry(*this, Anchor::Bottom, ta, t, m, c, p);
}
ConstraintSet& ConstraintSet::width(Anchor ta, Widget* t, float m, float c, float p) {
    return setEntry(*this, Anchor::Width, ta, t, m, c, p);
}
ConstraintSet& ConstraintSet::height(Anchor ta, Widget* t, float m, float c, float p) {
    return setEntry(*this, Anchor::Height, ta, t, m, c, p);
}
ConstraintSet& ConstraintSet::centerX(Anchor ta, Widget* t, float m, float c, float p) {
    return setEntry(*this, Anchor::CenterX, ta, t, m, c, p);
}
ConstraintSet& ConstraintSet::centerY(Anchor ta, Widget* t, float m, float c, float p) {
    return setEntry(*this, Anchor::CenterY, ta, t, m, c, p);
}

// ––––––––[ global storage ] per-widget constraints
ConstraintSet& constrain(Widget& w) noexcept {
    return g_constraints[&w];
}
const ConstraintSet* constraintsOf(const Widget& w) noexcept {
    auto it = g_constraints.find(const_cast<Widget*>(&w));
    if (it == g_constraints.end()) return nullptr;
    return &it->second;
}
void clearConstraints(Widget& w) noexcept {
    g_constraints.erase(&w);
}

} // namespace fluffy
