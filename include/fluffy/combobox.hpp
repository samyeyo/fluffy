// include/fluffy/combobox.hpp
// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
// ComboBox — dropdown selection (button + popup list)
// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
#pragma once
#include <fluffy/animation.hpp>
#include <fluffy/inline.hpp>
#include <fluffy/widget.hpp>
#include <functional>
#include <string>
#include <vector>
namespace fluffy {
class ComboBox final : public Widget {
public:
    using ChangeHandler = std::function<void(int index, const std::string& value)>;

    explicit ComboBox(Rect bounds = Rect{});

    // ––––––––[ setItems ] replaces item list, resets selection if out of range
    void setItems(std::vector<std::string> items);
    // ––––––––[ setItemsInline ] replaces item list with Inline items
    void setItemsInline(std::vector<Inline> items);

    // ––––––––[ setSelectedIndex ] selects by index (-1 = none)
    void setSelectedIndex(int idx);

    // ––––––––[ setPlaceholder ] text when nothing selected
    void setPlaceholder(std::string p);
    // ––––––––[ setPlaceholderInline ] inline placeholder
    void setPlaceholderInline(Inline p);

    // ––––––––[ setOnChange ] fired when selection changes
    void setOnChange(ChangeHandler h) { on_change_ = std::move(h); }
    // ––––––––[ onChange ] chainable selection handler, preferred over setOnChange
    ComboBox& onChange(ChangeHandler h) { on_change_ = std::move(h); return *this; }
    // ––––––––[ setText ] unified setter shows a single value without a dropdown
    void setText(Inline v) { inlineItems_ = {std::move(v)}; selected_ = 0; }

    int selectedIndex() const noexcept { return selected_; }
    const std::string& selectedValue() const noexcept;
    Inline selectedInline() const noexcept;
    const std::string& placeholder() const noexcept { return placeholder_; }
    Inline placeholderInline() const noexcept { return placeholderInline_; }
    const std::vector<std::string>& items() const noexcept { return items_; }
    const std::vector<Inline>& itemsInline() const noexcept { return inlineItems_; }

    void draw(NVGcontext* vg, const Theme& theme, const Window& w) override;
    EventResult handleEvent(const Event& e, const Window& w) override;
    void tick(float dt, const Theme& theme) noexcept override;
    void updateHover(float mx, float my) noexcept override;
    bool isFocusable() const noexcept override { return true; }
    void setFocused(bool f) noexcept override;
    void setProperty(std::string_view name, const YamlNode& value) override;
    float naturalWidth(const Theme& theme, const Window& w) const override;
    float naturalHeight(const Theme& theme, const Window& w) const override;
    void openPopup(const Window& w) { showPopup(w); }

private:
    // ––––––––[ items_ ] option strings (kept for compat, derived from inlineItems_)
    std::vector<std::string> items_;
    // ––––––––[ inlineItems_ ] Inline option items (colors/glyphs)
    std::vector<Inline> inlineItems_;
    // ––––––––[ selected_ ] -1 = none
    int selected_ = -1;
    // ––––––––[ placeholder_ ] shown when selected == -1
    std::string placeholder_ = "Select…";
    // ––––––––[ placeholderInline_ ] inline placeholder
    Inline placeholderInline_;
    // ––––––––[ hovered_ ] mouse over
    bool hovered_ = false;
    // ––––––––[ pressed_ ] mouse down
    bool pressed_ = false;
    // ––––––––[ focused_ ] keyboard focus
    bool focused_ = false;
    // ––––––––[ hover_anim_ ] hover transition
    SmoothAnim hover_anim_;
    // ––––––––[ press_anim_ ] press transition
    SmoothAnim press_anim_;
    // ––––––––[ on_change_ ] selection callback
    ChangeHandler on_change_;
    // ––––––––[ hadPopup_ ] popup was open last frame (to detect Window pre-close)
    bool hadPopup_ = false;
    // ––––––––[ suppressNextOpen_ ] suppress reopen after toggle-close
    bool suppressNextOpen_ = false;

    void showPopup(const Window& w);
};
}  // namespace fluffy
