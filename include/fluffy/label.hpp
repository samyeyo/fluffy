// include/fluffy/label.hpp

// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

// Label — static non-interactive text widget (std::string or Inline source).

#pragma once
#include <fluffy/inline.hpp>
#include <fluffy/widget.hpp>
#include <string>
#include <vector>
namespace fluffy {
class Label final : public Widget {
public:
    enum class Align { Left, Center, Right };
    Label() = default;
    explicit Label(std::string text,
                   Align horizontal = Align::Left,
                   Align vertical   = Align::Center)
        : Label(Rect{}, std::move(text), horizontal, vertical) {}
    explicit Label(Inline content,
                   Align horizontal = Align::Left,
                   Align vertical   = Align::Center)
        : Label(Rect{}, std::move(content), horizontal, vertical) {}
    Label(Rect bounds,
          std::string text,
          Align horizontal = Align::Left,
          Align vertical   = Align::Center);
    Label(Rect bounds,
          Inline content,
          Align horizontal = Align::Left,
          Align vertical   = Align::Center);
    void draw(NVGcontext* vg, const Theme& theme, const Window& w) override;
    EventResult handleEvent(const Event& e, const Window&) override {
        (void)e;
        return EventResult::Ignored;
    }
    void setProperty(std::string_view name, const YamlNode& value) override;
    Label& setTextSize(float size) noexcept { Widget::setTextSize(size); return *this; }
    float naturalWidth(const Theme& theme, const Window& w) const override;
    float naturalHeight(const Theme& theme, const Window& w) const override;
    const std::string& text() const noexcept { return text_; }
    void               setText(std::string t);
    void               setInline(Inline c);
    // ––––––––[ setContent ] unified setter accepts plain strings and inline content
    Label& setContent(Inline c) { setInline(std::move(c)); return *this; }
    Align  horizontal() const noexcept { return h_; }
    Label& setHorizontal(Align a) noexcept { h_ = a; return *this; }
    Align  vertical()   const noexcept { return v_; }
    Label& setVertical(Align a) noexcept { v_ = a; return *this; }
    bool   wordWrap() const noexcept { return word_wrap_; }
    Label& setWordWrap(bool w) noexcept { word_wrap_ = w; return *this; }
private:
    std::string              text_;
    std::vector<Inline::Run> inline_;
    bool                     is_inline_ = false;
    bool                     word_wrap_ = true;
    Align h_ = Align::Left;
    Align v_ = Align::Center;
};
}  

