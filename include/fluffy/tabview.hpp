// include/fluffy/tabview.hpp
// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
// TabView — header tabs + content area
// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
#pragma once
#include <fluffy/container.hpp>
#include <fluffy/widget.hpp>
#include <functional>
#include <memory>
#include <string>
#include <vector>
namespace fluffy {
class TabView final : public Widget {
public:
    struct Tab {
        std::string label;
        std::string id;
    };

    explicit TabView(Rect bounds = Rect{});

    // ––––––––[ addTab ] appends tab
    void addTab(std::string label, std::string id = "");

    // ––––––––[ setSelected ] selects by index
    void setSelected(int idx);

    // ––––––––[ setOnSelect ] callback
    void setOnSelect(std::function<void(int, const std::string&)> h) { on_select_ = std::move(h); }
    // ––––––––[ onSelect ] chainable selection handler, preferred over setOnSelect
    TabView& onSelect(std::function<void(int, const std::string&)> h) { on_select_ = std::move(h); return *this; }

    int selected() const noexcept { return selected_; }

    void draw(NVGcontext* vg, const Theme& theme, const Window& w) override;
    EventResult handleEvent(const Event& e, const Window& w) override;
    void tick(float dt, const Theme& theme) noexcept override;
    void updateHover(float mx, float my) noexcept override;
    void setProperty(std::string_view name, const YamlNode& value) override;
    float naturalWidth(const Theme& theme, const Window& w) const override;
    float naturalHeight(const Theme& theme, const Window& w) const override;
    void setWindow(Window* w) noexcept override;
    bool isFocusable() const noexcept override { return true; }
    void setFocused(bool f) noexcept override { focused_ = f; }

    // ––––––––[ content ] container for selected tab's widgets
    Container* content() noexcept { return content_.get(); }
    void forEachChild(const std::function<void(Widget*)>& fn) override {
        if (content_) fn(content_.get());
    }

private:
    // ––––––––[ tabs_ ] tab headers
    std::vector<Tab> tabs_;
    // ––––––––[ selected_ ] index
    int selected_ = -1;
    // ––––––––[ hovered_ ] hovered tab
    int hovered_ = -1;
    // ––––––––[ content_ ] content container
    std::unique_ptr<Container> content_;
    // ––––––––[ on_select_ ] callback
    std::function<void(int, const std::string&)> on_select_;
    // ––––––––[ tab_rects_ ] computed header rects
    std::vector<Rect> tab_rects_;
    // ––––––––[ focused_ ] focus state
    bool focused_ = false;
    void rebuildRects(const Theme& theme, const Window& w);
};
}  // namespace fluffy
