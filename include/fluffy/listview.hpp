// include/fluffy/listview.hpp
// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
// ListView — scrollable vertical list with selection
// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
#pragma once
#include <fluffy/animation.hpp>
#include <fluffy/inline.hpp>
#include <fluffy/widget.hpp>
#include <functional>
#include <string>
#include <vector>
namespace fluffy {
class ListView final : public Widget {
public:
    using SelectHandler = std::function<void(int index, const std::string& value)>;

    explicit ListView(Rect bounds = Rect{});

    // ––––––––[ setItems ] replaces items
    void setItems(std::vector<std::string> items);
    // ––––––––[ setItemsInline ] replaces items with Inline
    void setItemsInline(std::vector<Inline> items);

    // ––––––––[ setSelected ] selects index
    void setSelected(int idx);

    // ––––––––[ setOnSelect ] selection callback
    void setOnSelect(SelectHandler h) { on_select_ = std::move(h); }
    // ––––––––[ onSelect ] chainable selection handler, preferred over setOnSelect
    ListView& onSelect(SelectHandler h) { on_select_ = std::move(h); return *this; }

    int selected() const noexcept { return selected_; }
    const std::vector<std::string>& items() const noexcept { return items_; }
    const std::vector<Inline>& itemsInline() const noexcept { return inlineItems_; }
    Inline selectedInline() const noexcept;

    void draw(NVGcontext* vg, const Theme& theme, const Window& w) override;
    EventResult handleEvent(const Event& e, const Window& w) override;
    void tick(float dt, const Theme& theme) noexcept override;
    void updateHover(float mx, float my) noexcept override;
    bool isFocusable() const noexcept override { return true; }
    void setFocused(bool f) noexcept override;
    void setProperty(std::string_view name, const YamlNode& value) override;
    float naturalWidth(const Theme& theme, const Window& w) const override;
    float naturalHeight(const Theme& theme, const Window& w) const override;

private:
    // ––––––––[ items_ ] strings (compat)
    std::vector<std::string> items_;
    // ––––––––[ inlineItems_ ] Inline items (colors/glyphs)
    std::vector<Inline> inlineItems_;
    // ––––––––[ selected_ ] -1 none
    int selected_ = -1;
    // ––––––––[ hovered_idx_ ] hovered row
    int hovered_idx_ = -1;
    // ––––––––[ scroll_y_ ] vertical scroll
    float scroll_y_ = 0.0f;
    // ––––––––[ focused_ ] focus
    bool focused_ = false;
    // ––––––––[ row_h_ ] cached row height
    float row_h(const Theme& theme) const noexcept;
    // ––––––––[ on_select_ ] callback
    SelectHandler on_select_;
    // ––––––––[ hover_anim_ ] hover fade
    SmoothAnim hover_anim_;
    void ensureVisible(int idx, const Theme& theme);
};
}  // namespace fluffy
