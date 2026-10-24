// src/layout.cpp
// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
// Layout — apply() for HorizontalLayout and VerticalLayout.
//
// MIT License - Copyright (c) 2026 - Samir Tine
// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

#include <fluffy/layout.hpp>
#include <fluffy/yaml_fwd.hpp>
#include <algorithm>
#include <iterator>
namespace fluffy {
namespace {
bool ieq(std::string_view a, std::string_view b) noexcept {
    if (a.size() != b.size()) return false;
    for (std::size_t i = 0; i < a.size(); ++i) {
        char ca = a[i], cb = b[i];
        if (ca >= 'A' && ca <= 'Z') ca = char(ca - 'A' + 'a');
        if (cb >= 'A' && cb <= 'Z') cb = char(cb - 'A' + 'b');
        if (ca != cb) return false;
    }
    return true;
}
bool parseAlign(std::string_view s, Align& out) noexcept {
    if (ieq(s, "start")  || ieq(s, "left")  || ieq(s, "top"))    { out = Align::Start;  return true; }
    if (ieq(s, "center") || ieq(s, "centre") || ieq(s, "middle")) { out = Align::Center; return true; }
    if (ieq(s, "end")    || ieq(s, "right") || ieq(s, "bottom"))  { out = Align::End;    return true; }
    return false;
}
bool parseJustify(std::string_view s, Justify& out) noexcept {
    if (ieq(s, "start"))         { out = Justify::Start;       return true; }
    if (ieq(s, "center"))        { out = Justify::Center;       return true; }
    if (ieq(s, "end"))           { out = Justify::End;          return true; }
    if (ieq(s, "space-between")) { out = Justify::SpaceBetween; return true; }
    if (ieq(s, "space-evenly"))  { out = Justify::SpaceEvenly;  return true; }
    return false;
}
}  
void Layout::setProperty(std::string_view name, const YamlNode& value) {
    if (name == "spacing")           { spacing_  = value.asFloat(); return; }
    if (name == "padding")           { padding_  = value.asFloat(); return; }
    if (name == "margin")            { margin_   = value.asFloat(); return; }
    if (name == "align" || name == "align_items") {
        const std::string raw = value.asString();
        Align target;
        if (parseAlign(raw, target)) { align_ = target; }
        return;
    }
    if (name == "justify" || name == "justify_content") {
        const std::string raw = value.asString();
        Justify target;
        if (parseJustify(raw, target)) { justify_ = target; }
        return;
    }
}
float Layout::crossOffset(float slot_size, float child_size) const noexcept {
    const float free = std::max(0.0f, slot_size - child_size);
    switch (align_) {
    case Align::Start:  return 0.0f;
    case Align::Center: return free * 0.5f;
    case Align::End:    return free;
    }
    return 0.0f;
}
void HorizontalLayout::apply(const Rect& b,
                             std::vector<std::unique_ptr<Widget>>& children,
                             const Theme& theme) {
    if (children.empty()) return;
    const float usable_w   = std::max(0.0f, b.w - 2.0f * padding_);
    const float gap_total  = spacing_ * static_cast<float>(children.size() - 1);
    const float each_w     = (usable_w - gap_total) /
                             static_cast<float>(children.size());
    const float slot_y = b.y + padding_;
    const float slot_h = std::max(0.0f, b.h - 2.0f * padding_);
    const float each_h = std::max(0.0f, slot_h - 2.0f * margin_);
    // First pass: compute child sizes
    struct ChildLayout { float w, h; };
    std::vector<ChildLayout> sizes(children.size());
    float total_nat_w = 0.0f;
    for (std::size_t i = 0; i < children.size(); ++i) {
        const auto& c = children[i];
        const bool autoW = (c->autoSize() == AutoSize::Width ||
                            c->autoSize() == AutoSize::Both);
        const float slot_w = std::max(0.0f, each_w - 2.0f * margin_);
        float child_w = slot_w;
        float child_h = each_h;
        if (autoW && c->window()) {
            child_w = (std::min)(slot_w, c->naturalWidth(theme, *c->window()));
        }
        sizes[i] = {std::max(1.0f, child_w), std::max(1.0f, child_h)};
        total_nat_w += sizes[i].w;
    }
    // Compute leftover space for justify
    const float leftover = std::max(0.0f, usable_w - gap_total - total_nat_w);
    float x_start = b.x + padding_;
    if (justify_ == Justify::Center) {
        x_start += leftover * 0.5f;
    } else if (justify_ == Justify::End) {
        x_start += leftover;
    }
    // SpaceBetween and SpaceEvenly adjust gaps instead
    float adj_spacing = spacing_;
    if (justify_ == Justify::SpaceBetween && children.size() > 1) {
        adj_spacing = leftover / static_cast<float>(children.size() - 1);
    } else if (justify_ == Justify::SpaceEvenly) {
        const float gap = leftover / static_cast<float>(children.size() + 1);
        x_start += gap;
        adj_spacing = gap;
    }
    float x = x_start;
    for (std::size_t i = 0; i < children.size(); ++i) {
        auto& c = children[i];
        const float y_cross = crossOffset(each_h, sizes[i].h);
        c->setBounds(Rect{x + margin_, slot_y + margin_ + y_cross, sizes[i].w, sizes[i].h});
        x += sizes[i].w + adj_spacing;
    }
}
void VerticalLayout::apply(const Rect& b,
                           std::vector<std::unique_ptr<Widget>>& children,
                           const Theme& theme) {
    if (children.empty()) return;
    const float usable_h   = std::max(0.0f, b.h - 2.0f * padding_);
    const float slot_x = b.x + padding_;
    const float slot_w = std::max(0.0f, b.w - 2.0f * padding_);
    const float each_w = std::max(0.0f, slot_w - 2.0f * margin_);
    const float gap_total = spacing_ * static_cast<float>(children.size() - 1);
    // First pass: collect natural heights for autoSized children
    float total_nat = 0.0f;
    for (std::size_t i = 0; i < children.size(); ++i) {
        const auto& c = children[i];
        const bool autoH = (c->autoSize() == AutoSize::Height ||
                            c->autoSize() == AutoSize::Both);
        if (autoH && c->window()) {
            total_nat += c->naturalHeight(theme, *c->window());
        }
    }
    // Non-autoSized children share the remaining space proportionally by stretch weight.
    const float non_auto_space = usable_h - gap_total - total_nat;
    float total_weight = 0.0f;
    for (const auto& c : children) {
        const bool autoH = (c->autoSize() == AutoSize::Height ||
                            c->autoSize() == AutoSize::Both);
        if (!autoH) total_weight += c->stretchWeight();
    }
    // Compute child heights and total used
    struct ChildLayout { float w, h; };
    std::vector<ChildLayout> sizes(children.size());
    float total_used = 0.0f;
    for (std::size_t i = 0; i < children.size(); ++i) {
        auto& c = children[i];
        const bool autoH = (c->autoSize() == AutoSize::Height ||
                            c->autoSize() == AutoSize::Both);
        float child_h;
        if (autoH && c->window()) {
            child_h = (std::max)(1.0f, c->naturalHeight(theme, *c->window()));
        } else {
            const float weight = c->stretchWeight();
            child_h = (total_weight > 0.0f)
                ? (std::max)(1.0f, non_auto_space * (weight / total_weight))
                : (std::max)(1.0f, non_auto_space / static_cast<float>(children.size()));
        }
        sizes[i] = {std::max(1.0f, each_w), std::max(1.0f, child_h)};
        total_used += sizes[i].h;
    }
    // Compute leftover space for justify
    const float leftover = std::max(0.0f, usable_h - gap_total - total_used);
    float y_start = b.y + padding_;
    if (justify_ == Justify::Center) {
        y_start += leftover * 0.5f;
    } else if (justify_ == Justify::End) {
        y_start += leftover;
    }
    float adj_spacing = spacing_;
    if (justify_ == Justify::SpaceBetween && children.size() > 1) {
        adj_spacing = leftover / static_cast<float>(children.size() - 1);
    } else if (justify_ == Justify::SpaceEvenly) {
        const float gap = leftover / static_cast<float>(children.size() + 1);
        y_start += gap;
        adj_spacing = gap;
    }
    float y = y_start;
    for (std::size_t i = 0; i < children.size(); ++i) {
        auto& c = children[i];
        const float x_cross = crossOffset(each_w, sizes[i].w);
        c->setBounds(Rect{
            slot_x + margin_ + x_cross,
            y + margin_,
            sizes[i].w,
            sizes[i].h
        });
        y += sizes[i].h + adj_spacing;
    }
}

// ––––––––[ GridLayout::apply ] grid with equal cells
void GridLayout::apply(const Rect& b,
                       std::vector<std::unique_ptr<Widget>>& children,
                       const Theme&) {
    if (children.empty() || cols_ <= 0) return;
    int rows = rows_;
    if (rows <= 0) rows = static_cast<int>((std::ssize(children) + cols_ - 1) / cols_);
    if (rows <= 0) rows = 1;
    const float usable_w = std::max(0.0f, b.w - 2.0f * padding_ - spacing_ * (cols_ - 1) - 2.0f * margin_ * cols_);
    const float usable_h = std::max(0.0f, b.h - 2.0f * padding_ - spacing_ * (rows - 1) - 2.0f * margin_ * rows);
    const float cell_w = usable_w / static_cast<float>(cols_);
    const float cell_h = usable_h / static_cast<float>(rows);
    for (std::ptrdiff_t i = 0; i < std::ssize(children); ++i) {
        int r = static_cast<int>(i) / cols_;
        int c = static_cast<int>(i) % cols_;
        if (r >= rows) break;
        float x = b.x + padding_ + margin_ + c * (cell_w + spacing_ + 2.0f * margin_);
        float y = b.y + padding_ + margin_ + r * (cell_h + spacing_ + 2.0f * margin_);
        children[i]->setBounds(Rect{x, y, std::max(1.0f, cell_w), std::max(1.0f, cell_h)});
    }
}

// ––––––––[ GridLayout::setProperty ] yaml support
void GridLayout::setProperty(std::string_view name, const YamlNode& value) {
    if (name == "cols" || name == "columns") { cols_ = std::max(1, value.asInt()); return; }
    if (name == "rows") { rows_ = std::max(0, value.asInt()); return; }
    Layout::setProperty(name, value);
}
}  
