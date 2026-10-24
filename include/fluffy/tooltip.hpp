// include/fluffy/tooltip.hpp
// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
// Tooltip — hover overlay text (global helper)
// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
#pragma once
#include <fluffy/animation.hpp>
#include <fluffy/inline.hpp>
#include <fluffy/widget.hpp>
#include <string>
namespace fluffy {
class Tooltip : public Widget {
public:
    explicit Tooltip(Rect bounds = Rect{}, std::string text = "");
    explicit Tooltip(Rect bounds, Inline inl);

    // ––––––––[ setText ] sets tooltip text
    void setText(std::string t);
    // ––––––––[ setInline ] sets tooltip inline
    void setInline(Inline inl);

    // ––––––––[ showFor ] shows tooltip near widget bounds (static helper)
    static void showFor(Widget* anchor, const std::string& text, Window& w);
    // ––––––––[ showForInline ] shows inline tooltip
    static void showFor(Widget* anchor, const Inline& inl, Window& w);

    void draw(NVGcontext* vg, const Theme& theme, const Window& w) override;
    EventResult handleEvent(const Event& e, const Window& w) override { (void)e; (void)w; return EventResult::Ignored; }
    void tick(float dt, const Theme& theme) noexcept override;
    float naturalWidth(const Theme& theme, const Window& w) const override;
    float naturalHeight(const Theme& theme, const Window& w) const override;
    bool isTooltip() const noexcept override { return true; }

private:
    // ––––––––[ text_ ] tooltip string (compat)
    std::string text_;
    // ––––––––[ inline_ ] inline content (colors/glyphs)
    Inline inline_;
    // ––––––––[ anchor_ ] widget that triggered tooltip
    Widget* anchor_ = nullptr;
    // ––––––––[ timer_ ] seconds since shown
    float timer_ = 0.0f;
    // ––––––––[ auto_hide_ ] seconds before auto-close
    float auto_hide_ = 2.5f;
    // ––––––––[ fade_anim_ ] smooth fade in/out
    SmoothAnim fade_anim_;
    // ––––––––[ closing_ ] fading out
    bool closing_ = false;
};

// ––––––––[ setTooltip ] attaches tooltip to widget (hover)
void setTooltip(Widget* w, std::string text);
// ––––––––[ setTooltipInline ] attaches inline tooltip
void setTooltip(Widget* w, Inline inl);
// ––––––––[ getInlineTooltip ] retrieves inline tooltip if any
Inline getInlineTooltip(Widget* w);

}  // namespace fluffy
