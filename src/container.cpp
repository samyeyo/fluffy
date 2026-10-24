// src/container.cpp

// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

// Container — draw / handleEvent / updateHover / tick / onResize delegation
//             with optional scrollable content.

#include <fluffy/container.hpp>
#include <fluffy/constraint.hpp>

#include <algorithm>

#include <nanovg.h>

#include <fluffy/event.hpp>
#include <fluffy/window.hpp>
#include <fluffy/yaml_fwd.hpp>

namespace fluffy {
namespace {

// ––––––––[ anchorOf ] reads one anchor value from an x/y/w/h box.
float anchorOf(Anchor a, float x, float y, float w, float h) noexcept {
    switch (a) {
    case Anchor::Left:
    case Anchor::Leading: return x;
    case Anchor::Right:
    case Anchor::Trailing: return x + w;
    case Anchor::Top: return y;
    case Anchor::Bottom: return y + h;
    case Anchor::Width: return w;
    case Anchor::Height: return h;
    case Anchor::CenterX: return x + w * 0.5f;
    case Anchor::CenterY: return y + h * 0.5f;
    case Anchor::Baseline: return y + h;
    default: return 0.0f;
    }
}

} // namespace

// ––––––––[ solveConstraints ] anchor solver; constrained children win over layout.
void Container::solveConstraints(const Rect& parentRect, const Theme& theme, const Window& win) {
    struct Box {
        Widget* widget = nullptr;
        size_t index = 0;
        float x = 0.0f, y = 0.0f, w = 0.0f, h = 0.0f;
        std::vector<std::pair<Anchor, Constraint>> ordered;
    };
    std::vector<Box> boxes;
    boxes.reserve(children_.size());
    for (size_t i = 0; i < children_.size(); ++i) {
        Widget* cw = children_[i].get();
        if (!cw) continue;
        const ConstraintSet* cs = constraintsOf(*cw);
        if (!cs || cs->empty()) continue;
        Box b;
        b.widget = cw;
        b.index = i;
        const Rect& r = cw->bounds();
        b.x = r.x; b.y = r.y; b.w = r.w; b.h = r.h;
        for (const auto& [a, c] : cs->entries) b.ordered.emplace_back(a, c);
        // Lower priority first so required constraints apply last and win.
        std::stable_sort(b.ordered.begin(), b.ordered.end(),
            [](const auto& l, const auto& r) { return l.second.priority < r.second.priority; });
        boxes.push_back(std::move(b));
    }
    if (boxes.empty()) return;

    auto findBox = [&](Widget* wgt) -> Box* {
        for (auto& b : boxes) {
            if (b.widget == wgt) return &b;
        }
        return nullptr;
    };

    // ––––––––[ resolveTarget ] parent rect, solved sibling state, or live bounds.
    auto resolveTarget = [&](const Constraint& c, const Box& self, Rect& out) -> bool {
        if (c.targetWidget) {
            if (Box* tb = findBox(c.targetWidget)) {
                out = Rect(tb->x, tb->y, tb->w, tb->h);
            } else {
                out = c.targetWidget->bounds();
            }
            return true;
        }
        if (c.targetName == "parent") { out = parentRect; return true; }
        if (c.targetName == "prev") {
            if (self.index == 0 || self.index > children_.size()) return false;
            Widget* prev = children_[self.index - 1].get();
            if (!prev) return false;
            if (Box* tb = findBox(prev)) {
                out = Rect(tb->x, tb->y, tb->w, tb->h);
            } else {
                out = prev->bounds();
            }
            return true;
        }
        return false;
    };

    auto applyOne = [&](Box& b, Anchor selfA, const Constraint& c) {
        float v = 0.0f;
        if (c.targetAnchor == Anchor::None) {
            v = c.constant;
        } else {
            Rect tr;
            if (!resolveTarget(c, b, tr)) return;
            v = anchorOf(c.targetAnchor, tr.x, tr.y, tr.w, tr.h) * c.multiplier + c.constant;
        }
        float cur = anchorOf(selfA, b.x, b.y, b.w, b.h);
        float nv = v;
        if (c.rel == Rel::GreaterEqual) nv = cur > v ? cur : v;
        else if (c.rel == Rel::LessEqual) nv = cur < v ? cur : v;
        switch (selfA) {
        case Anchor::Left:
        case Anchor::Leading: b.x = nv; break;
        case Anchor::Right:
        case Anchor::Trailing: b.x = nv - b.w; break;
        case Anchor::Top: b.y = nv; break;
        case Anchor::Bottom: b.y = nv - b.h; break;
        case Anchor::Width: b.w = nv; break;
        case Anchor::Height: b.h = nv; break;
        case Anchor::CenterX: b.x = nv - b.w * 0.5f; break;
        case Anchor::CenterY: b.y = nv - b.h * 0.5f; break;
        case Anchor::Baseline: b.y = nv - b.h; break;
        default: break;
        }
    };

    // Gauss-Seidel: a few passes converge pin/center/size chains.
    for (int pass = 0; pass < 4; ++pass) {
        for (auto& b : boxes) {
            for (const auto& [a, c] : b.ordered) applyOne(b, a, c);
        }
    }

    for (auto& b : boxes) {
        // Under-constrained size falls back to natural size.
        if (b.w <= 0.0f) {
            float nw = b.widget->naturalWidth(theme, win);
            if (nw > 0.0f) b.w = nw;
        }
        if (b.h <= 0.0f) {
            float nh = b.widget->naturalHeight(theme, win);
            if (nh > 0.0f) b.h = nh;
        }
        b.widget->setBounds(Rect(b.x, b.y, b.w, b.h));
    }
}

// ––––––––[ draw ] renders children, clipping to bounds when scrollable.
void Container::draw(NVGcontext* vg, const Theme& theme, const Window& w) {
    if (!visible_) return;

    // Shift layout bounds so children are positioned in scrolled coordinate space
    Rect layout_bounds = bounds_;
    if (scrollable_) {
        layout_bounds.x -= scroll_x_;
        layout_bounds.y -= scroll_y_;
    }

    if (layout_) layout_->apply(layout_bounds, children_, theme);
    // Constraints win over layout output.
    solveConstraints(layout_bounds, theme, w);

    if (scrollable_) {
        computeContentSize();
        clampScroll();
    }

    // Clip children to container's visible area
    if (scrollable_) {
        nvgSave(vg);
        nvgIntersectScissor(vg, bounds_.x, bounds_.y, bounds_.w, bounds_.h);
    }

    for (auto& c : children_) {
        if (c->visible()) c->draw(vg, theme, w);
    }

    if (scrollable_) {
        nvgRestore(vg);
        drawScrollIndicators(vg, theme);
    }
}

// ––––––––[ handleEvent ] scroll wheel adjusts offset; children receive raw coords.
EventResult Container::handleEvent(const Event& e, const Window& w) {
    // Scroll wheel
    if (scrollable_ && kind_of(e) == EventKind::MouseScroll) {
        const auto& se = std::get<MouseScrollEvent>(e);
        scroll_y_ -= se.dy * 40.0f;
        scroll_x_ += se.dx * 20.0f;
        clampScroll();
        return EventResult::Consumed;
    }

    // Delegate to children (bounds are already in scrolled space)
    for (auto it = children_.rbegin(); it != children_.rend(); ++it) {
        if ((*it)->visible() && (*it)->enabled() &&
            (*it)->handleEvent(e, w) == EventResult::Consumed) {
            return EventResult::Consumed;
        }
    }
    return EventResult::Ignored;
}

void Container::updateHover(float mx, float my) noexcept {
    for (auto& c : children_) {
        if (c->visible() && c->enabled()) c->updateHover(mx, my);
    }
}

void Container::tick(float dt, const Theme& theme) noexcept {
    for (auto& c : children_) {
        if (c->visible() && c->enabled()) c->tick(dt, theme);
    }
}

void Container::onResize(const ResizeEvent& e) {
    if (window_) {
        Rect layout_bounds = bounds_;
        if (scrollable_) {
            layout_bounds.x -= scroll_x_;
            layout_bounds.y -= scroll_y_;
        }
        if (layout_) layout_->apply(layout_bounds, children_, window_->theme());
        solveConstraints(layout_bounds, window_->theme(), *window_);
    }
    for (auto& c : children_) c->onResize(e);
}

float Container::naturalWidth(const Theme& theme, const Window& w) const {
    if (scrollable_) return bounds_.w;
    if (!layout_ || children_.empty()) return 0.0f;
    const float pad = layout_->padding();
    if (layout_->isHorizontal()) {
        float total = 2.0f * pad;
        for (auto& c : children_)
            total += c->naturalWidth(theme, w);
        total += layout_->spacing() * static_cast<float>(children_.size() - 1);
        return total;
    }
    float max_w = 0.0f;
    for (auto& c : children_)
        max_w = (std::max)(max_w, c->naturalWidth(theme, w));
    return max_w + 2.0f * pad;
}

float Container::naturalHeight(const Theme& theme, const Window& w) const {
    if (scrollable_) return bounds_.h;
    if (!layout_ || children_.empty()) return 0.0f;
    const float pad = layout_->padding();
    if (layout_->isHorizontal()) {
        float max_h = 0.0f;
        for (auto& c : children_)
            max_h = (std::max)(max_h, c->naturalHeight(theme, w));
        return max_h + 2.0f * pad;
    }
    float total = 2.0f * pad;
    for (auto& c : children_)
        total += c->naturalHeight(theme, w);
    total += layout_->spacing() * static_cast<float>(children_.size() - 1);
    return total;
}

void Container::setEvent(std::string_view name, std::string_view handlerName) {
    Widget::setEvent(name, handlerName);
}

// ––––––––[ setProperty ] supports "scrollable" plus base properties.
void Container::setProperty(std::string_view name, const YamlNode& value) {
    if (name == "scrollable") { scrollable_ = value.asBool(); return; }
    Widget::setProperty(name, value);
}

// ––––––––[ computeContentSize ] bounding box of all children relative to container origin.
// Children are positioned in scrolled space (layout_bounds shifted by -scroll),
// so we add back the scroll offset to get the true content size.
void Container::computeContentSize() {
    content_w_ = 0.0f;
    content_h_ = 0.0f;
    for (const auto& c : children_) {
        const Rect& b = c->bounds();
        const float right  = b.x + b.w - bounds_.x + scroll_x_;
        const float bottom = b.y + b.h - bounds_.y + scroll_y_;
        content_w_ = (std::max)(content_w_, right);
        content_h_ = (std::max)(content_h_, bottom);
    }
}

// ––––––––[ clampScroll ] keeps offsets within valid range.
void Container::clampScroll() {
    const float max_x = (std::max)(0.0f, content_w_ - bounds_.w);
    const float max_y = (std::max)(0.0f, content_h_ - bounds_.h);
    scroll_x_ = (std::clamp)(scroll_x_, 0.0f, max_x);
    scroll_y_ = (std::clamp)(scroll_y_, 0.0f, max_y);
}

// ––––––––[ drawScrollIndicators ] thin rounded-rect thumbs at right / bottom edges.
void Container::drawScrollIndicators(NVGcontext* vg, const Theme& theme) {
    (void)theme;
    // Only draw when content overflows
    const bool v_scroll = content_h_ > bounds_.h + 1.0f;
    const bool h_scroll = content_w_ > bounds_.w + 1.0f;
    if (!v_scroll && !h_scroll) return;

    const NVGcolor thumb_col = nvgRGBA(128, 128, 128, 80);

    // Vertical thumb
    if (v_scroll) {
        const float max_y  = content_h_ - bounds_.h;
        const float thumb_h = bounds_.h * (bounds_.h / content_h_);
        const float thumb_y = bounds_.y
            + (bounds_.h - thumb_h) * (scroll_y_ / max_y);
        nvgBeginPath(vg);
        nvgRoundedRect(vg,
            bounds_.x + bounds_.w - 4.0f, thumb_y,
            3.0f, thumb_h, 1.5f);
        nvgFillColor(vg, thumb_col);
        nvgFill(vg);
    }

    // Horizontal thumb
    if (h_scroll) {
        const float max_x  = content_w_ - bounds_.w;
        const float thumb_w = bounds_.w * (bounds_.w / content_w_);
        const float thumb_x = bounds_.x
            + (bounds_.w - thumb_w) * (scroll_x_ / max_x);
        nvgBeginPath(vg);
        nvgRoundedRect(vg,
            thumb_x, bounds_.y + bounds_.h - 4.0f,
            thumb_w, 3.0f, 1.5f);
        nvgFillColor(vg, thumb_col);
        nvgFill(vg);
    }
}

}  // namespace fluffy
