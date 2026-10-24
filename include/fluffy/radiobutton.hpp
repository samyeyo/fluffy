// include/fluffy/radiobutton.hpp

// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

// RadioButton — toggleable circular indicator with text label.

#pragma once
#include <fluffy/animation.hpp>
#include <fluffy/widget.hpp>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>
namespace fluffy {

// ––––––––[ RadioButton ] circular radio indicator with group-based mutual exclusion.
class RadioButton final : public Widget {
public:
    using ChangeHandler = std::function<void(bool)>;

    RadioButton(Rect bounds = Rect{}, std::string label = {});
    ~RadioButton() override { unregisterGroup(); }

    void         draw(NVGcontext* vg, const Theme& theme, const Window& w) override;
    EventResult  handleEvent(const Event& e, const Window& w) override;
    void         updateHover(float mx, float my) noexcept override;
    void         tick(float dt, const Theme& theme) noexcept override;
    void         setProperty(std::string_view name, const YamlNode& value) override;
    float        naturalWidth(const Theme& theme, const Window& w) const override;
    float        naturalHeight(const Theme& theme, const Window& w) const override;

    bool  isChecked() const noexcept { return checked_; }
    void  setChecked(bool c) noexcept;
    bool  isFocusable() const noexcept override { return true; }
    void  setFocused(bool f) noexcept override { focused_ = f; }
    bool  isFocused() const noexcept { return focused_; }
    void  setOnChange(ChangeHandler h) { on_change_ = std::move(h); }
    // ––––––––[ onChange ] chainable change handler, preferred over setOnChange
    RadioButton& onChange(ChangeHandler h) { on_change_ = std::move(h); return *this; }
    const std::string& label() const noexcept { return label_; }
    void  setLabel(std::string l) { label_ = std::move(l); }
    void  setGroup(std::string g) noexcept;
    const std::string& group() const noexcept { return group_; }

private:
    void registerGroup();
    void unregisterGroup();

    std::string   group_;
    std::string   label_;
    bool          checked_   = false;
    bool          hovered_   = false;
    bool          pressed_   = false;
    bool          focused_   = false;
    SmoothAnim    hoverAnim_;
    SmoothAnim    pressAnim_;
    ChangeHandler on_change_;

    static std::unordered_map<std::string, std::vector<RadioButton*>> s_groups;
};

} // namespace fluffy
