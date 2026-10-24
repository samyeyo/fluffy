// include/fluffy/inline.hpp

// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

// Inline — tokenised formatting stream (Style, Color, icon and text runs).

#pragma once
#include <fluffy/geom.hpp>
#include <fluffy/svg_icon.hpp>
#include <fluffy/theme.hpp>
#include <cstdint>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
struct NVGcontext;
namespace fluffy {
using Icons::SvgIconRef;
struct Style {
    std::optional<Color> fg = std::nullopt;
    float                size = 0.0f;
    bool                 bold = false;
    bool                 italic = false;
};
struct ResetStyle {};
class Inline {
public:       
        struct Run {
        enum class Type { Text, SvgIcon };
        Type                 type = Type::Text;
        std::string          utf8;
        SvgIconRef           svg_icon;
        std::optional<Color> color;
        float                size = 0.0f;
        bool                 bold   = false;
        bool                 italic = false;
    };
    Inline() = default;
    // ––––––––[ Inline ] explicit plain-text construction from any string kind
    explicit Inline(std::string_view text) { *this << text; }
    // ––––––––[ Inline ] explicit c-string construction, null-safe
    explicit Inline(const char* text) { *this << text; }
    // ––––––––[ Inline ] explicit std string construction
    explicit Inline(const std::string& text) { *this << std::string_view(text); }
    // ––––––––[ text ] named constructor for plain text without stream syntax
    static Inline text(std::string_view t) { return Inline(t); }
    Inline(const Inline&)                = default;
    Inline(Inline&&) noexcept            = default;
    Inline& operator=(const Inline&)     = default;
    Inline& operator=(Inline&&) noexcept = default;
    Inline& operator<<(const Style& s) {
        if (s.fg) current_style_.fg = *s.fg;
        else      current_style_.fg.reset();
        current_style_.size   = s.size;
        current_style_.bold   = s.bold;
        current_style_.italic = s.italic;
        return *this;
    }
    Inline& operator<<(ResetStyle) {
        current_style_.fg.reset();
        current_style_.size   = 0.0f;
        current_style_.bold   = false;
        current_style_.italic = false;
        return *this;
    }
    Inline& operator<<(const Color& c) {
        current_style_.fg = c;
        return *this;
    }
    Inline& operator<<(SvgIconRef svg) {
        runs_.push_back(Run{
            Run::Type::SvgIcon,
            std::string{},
            svg,
            current_style_.fg,
            current_style_.size,
            current_style_.bold,
            current_style_.italic,
        });
        return *this;
    }
    Inline& operator<<(std::string_view text) {
        if (text.empty()) return *this;
        runs_.push_back(Run{
            Run::Type::Text,
            std::string{text},
            SvgIconRef{nullptr, 0},
            current_style_.fg,
            current_style_.size,
            current_style_.bold,
            current_style_.italic,
        });
        return *this;
    }
    Inline& operator<<(const char* text) {
        return *this << std::string_view(text ? text : "");
    }
    const std::vector<Run>& runs() const noexcept { return runs_; }
    bool                   empty() const noexcept { return runs_.empty(); }

    // ––––––––[ takeRuns ] moves the run list out of this inline (no copy)
    std::vector<Run> takeRuns() { return std::move(runs_); }
private:
    std::vector<Run> runs_;
    Style            current_style_{};
};
void renderInline(
    NVGcontext* vg,
    const std::vector<Inline::Run>& runs,
    float anchor_x, float anchor_y,
    int   hflags, int   vflags,
    float base_size,
    float icon_size_gain,
    float icon_padding,
    const char* default_face,
    Color default_text_color,
    Color default_icon_color);
// ––––––––[ drawFocusRing ] draws an accent-coloured outline around the widget bounds.
void drawFocusRing(NVGcontext* vg, const Rect& bounds, const Theme& theme);
}  

// $(...) — shorthand macro for creating an Inline stream.
// Usage: $(Icons::check << " Done") instead of Inline{} << Icons::check << " Done"
#define $(...) Inline{} << __VA_ARGS__
