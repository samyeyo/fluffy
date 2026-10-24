// include/fluffy/container.hpp

// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

// Container — Widget with Layout + children, recurses hooks to descendants.

#pragma once
#include <fluffy/layout.hpp>
#include <fluffy/widget.hpp>
#include <concepts>
#include <memory>
#include <string_view>
#include <vector>
namespace fluffy {
class Container final : public Widget {
public:
    // ––––––––[ Container ] bounds default to empty, layout assigns geometry
    explicit Container(Rect bounds = Rect{}) : Widget(bounds) {}
    void setLayout(std::unique_ptr<Layout> layout) noexcept {
        layout_ = std::move(layout);
    }
    Layout* layout() const noexcept { return layout_.get(); }
    Container& setPadding(float p) noexcept { if (layout_) layout_->setPadding(p); return *this; }
    Container& setSpacing(float s) noexcept { if (layout_) layout_->setSpacing(s); return *this; }
    Container& align(Align a) noexcept { if (layout_) layout_->align(a); return *this; }
    Container& justify(Justify j) noexcept { if (layout_) layout_->justify(j); return *this; }
    void addChild(std::unique_ptr<Widget> child) {
        if (!child) return;
        child->setWindow(this->window_);
        children_.push_back(std::move(child));
    }
    // ––––––––[ add ] creates a child widget and returns a reference
    template <std::derived_from<Widget> T, typename... Args>
    T& add(Args&&... args) {
        auto ptr = std::make_unique<T>(std::forward<Args>(args)...);
        T& ref = *ptr;
        addChild(std::move(ptr));
        return ref;
    }
    // ––––––––[ setVertical ] installs a vertical layout in one call
    Container& setVertical(float spacing = 8.0f, float padding = 0.0f) {
        auto l = std::make_unique<VerticalLayout>();
        l->setSpacing(spacing);
        l->setPadding(padding);
        setLayout(std::move(l));
        return *this;
    }
    // ––––––––[ setHorizontal ] installs a horizontal layout in one call
    Container& setHorizontal(float spacing = 8.0f, float padding = 0.0f) {
        auto l = std::make_unique<HorizontalLayout>();
        l->setSpacing(spacing);
        l->setPadding(padding);
        setLayout(std::move(l));
        return *this;
    }
    // ––––––––[ setGrid ] installs a grid layout with a column count
    Container& setGrid(int cols = 2, float spacing = 8.0f, float padding = 0.0f) {
        auto l = std::make_unique<GridLayout>(cols);
        l->setSpacing(spacing);
        l->setPadding(padding);
        setLayout(std::move(l));
        return *this;
    }
    // ––––––––[ addRow ] nested horizontal container with auto height
    Container* addRow(float spacing = 6.0f) {
        auto row = std::make_unique<Container>();
        row->setHorizontal(spacing);
        row->setAutoSize(AutoSize::Height);
        Container* raw = row.get();
        addChild(std::move(row));
        return raw;
    }
    // ––––––––[ addColumn ] nested vertical container with auto height
    Container* addColumn(float spacing = 6.0f) {
        auto col = std::make_unique<Container>();
        col->setVertical(spacing);
        col->setAutoSize(AutoSize::Height);
        Container* raw = col.get();
        addChild(std::move(col));
        return raw;
    }
    std::size_t childCount() const noexcept { return children_.size(); }
    Widget* child(std::size_t i) const noexcept {
        return i < children_.size() ? children_[i].get() : nullptr;
    }
    // ––––––––[ removeAllChildren ] destroys all child widgets.
    void removeAllChildren() noexcept { children_.clear(); }
    void draw(NVGcontext* vg, const Theme& theme, const Window& w) override;
    EventResult handleEvent(const Event& e, const Window& w) override;
    void updateHover(float mx, float my) noexcept override;
    void tick(float dt, const Theme& theme) noexcept override;
    void onResize(const ResizeEvent& e) override;
    float naturalWidth(const Theme& theme, const Window& w) const override;
    float naturalHeight(const Theme& theme, const Window& w) const override;
    void clearFocus() noexcept override {
        setFocused(false);
        for (auto& c : children_) c->clearFocus();
    }
    void setWindow(Window* w) noexcept override {
        Widget::setWindow(w);
        for (auto& c : children_) c->setWindow(w);
    }
    void setEvent(std::string_view name, std::string_view handlerName) override;
    void setProperty(std::string_view name, const YamlNode& value) override;
    // ––––––––[ fillsParent ] a container with no explicit size fills the window
    bool fillsParent() const noexcept override {
        return bounds_.w <= 0.0f && bounds_.h <= 0.0f;
    }
    bool isContainer() const noexcept override { return true; }
    void forEachChild(const std::function<void(Widget*)>& fn) override {
        for (auto& c : children_) if (c) fn(c.get());
    }
    // ––––––––[ scrollable ] enable scroll wheel + scrollbar indicators.
    bool  isScrollable() const noexcept { return scrollable_; }
    Container& setScrollable(bool s) noexcept { scrollable_ = s; return *this; }
    float scrollX() const noexcept { return scroll_x_; }
    float scrollY() const noexcept { return scroll_y_; }
    Container& setScrollX(float x) noexcept { scroll_x_ = x; clampScroll(); return *this; }
    Container& setScrollY(float y) noexcept { scroll_y_ = y; clampScroll(); return *this; }
private:
    void computeContentSize();
    void clampScroll();
    void drawScrollIndicators(NVGcontext* vg, const Theme& theme);
    // ––––––––[ solveConstraints ] anchor solver; constrained children win over layout.
    void solveConstraints(const Rect& parentRect, const Theme& theme, const Window& win);
    std::unique_ptr<Layout>                  layout_;
    std::vector<std::unique_ptr<Widget>>     children_;
    bool   scrollable_ = false;
    float  scroll_x_   = 0.0f;
    float  scroll_y_   = 0.0f;
    float  content_w_  = 0.0f;
    float  content_h_  = 0.0f;
};
}  

