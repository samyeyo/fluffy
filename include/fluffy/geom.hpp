// include/fluffy/geom.hpp

// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

// Geom — Rect, Color and interpolation helpers (mix / blend3).

#pragma once
#include <algorithm>   
#include <cmath>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
namespace fluffy {
struct Rect {
    float x{0.0f}, y{0.0f}, w{0.0f}, h{0.0f};
    constexpr Rect() noexcept = default;
    constexpr Rect(float x_, float y_, float w_, float h_) noexcept
        : x{x_}, y{y_}, w{w_}, h{h_}
    {}
    constexpr float left()   const noexcept { return x; }
    constexpr float top()    const noexcept { return y; }
    constexpr float right()  const noexcept { return x + w; }
    constexpr float bottom() const noexcept { return y + h; }
    constexpr float width()  const noexcept { return w; }
    constexpr float height() const noexcept { return h; }
    constexpr float center_x() const noexcept { return x + w * 0.5f; }
    constexpr float center_y() const noexcept { return y + h * 0.5f; }
    constexpr bool empty() const noexcept { return w <= 0.0f || h <= 0.0f; }
    constexpr bool containsPoint(float px, float py) const noexcept {
        return px >= x && px < right() && py >= y && py < bottom();
    }
};
struct Color {
    std::uint8_t r{0}, g{0}, b{0}, a{255};
    constexpr Color() noexcept = default;
    constexpr Color(std::uint8_t r_, std::uint8_t g_, std::uint8_t b_,
                    std::uint8_t a_ = 255) noexcept
        : r{r_}, g{g_}, b{b_}, a{a_}
    {}
    static constexpr Color fromHex(std::uint32_t argb) noexcept {
        return Color{
            static_cast<std::uint8_t>((argb >> 16) & 0xFFu),
            static_cast<std::uint8_t>((argb >>  8) & 0xFFu),
            static_cast<std::uint8_t>((argb >>  0) & 0xFFu),
            static_cast<std::uint8_t>((argb >> 24) & 0xFFu)
        };
    }
    constexpr Color withAlpha(std::uint8_t new_alpha) const noexcept {
        return Color{r, g, b, new_alpha};
    }
    static std::optional<Color> fromString(std::string_view s) noexcept;
};
inline Color mix(Color a, Color b, float t) noexcept {
    const auto mix_ch = [t](std::uint8_t x, std::uint8_t y) -> std::uint8_t {
        return static_cast<std::uint8_t>(std::clamp(
            static_cast<float>(x) +
                (static_cast<float>(y) - static_cast<float>(x)) * t,
            0.0f, 255.0f));
    };
    return Color{mix_ch(a.r, b.r), mix_ch(a.g, b.g),
                 mix_ch(a.b, b.b), mix_ch(a.a, b.a)};
}
inline float mix(float a, float b, float t) noexcept {
    return a + (b - a) * t;
}
template <typename T>
inline T blend3(const T& a, const T& b, const T& c,
                float t1, float t2) noexcept {
    return mix(mix(a, b, t1), c, t2);
}
// ––––––––[ hsvToRgb ] HSV → RGB (h: 0-360, s: 0-1, v: 0-1)
inline Color hsvToRgb(float h, float s, float v) noexcept {
    if (s <= 0.0f) {
        const auto b = static_cast<std::uint8_t>(std::clamp(v * 255.0f, 0.0f, 255.0f) + 0.5f);
        return Color{b, b, b, 255};
    }
    h = std::fmod(h, 360.0f);
    if (h < 0.0f) h += 360.0f;
    const float sector = h / 60.0f;
    const int sextant = static_cast<int>(sector) % 6;
    const float c = v * s;
    const float x = c * (1.0f - std::abs(std::fmod(sector, 2.0f) - 1.0f));
    const float m = v - c;
    float r, g, b;
    switch (sextant) {
        case 0: r = c; g = x; b = 0.0f; break;
        case 1: r = x; g = c; b = 0.0f; break;
        case 2: r = 0.0f; g = c; b = x; break;
        case 3: r = 0.0f; g = x; b = c; break;
        case 4: r = x; g = 0.0f; b = c; break;
        default: r = c; g = 0.0f; b = x; break;
    }
    return Color{
        static_cast<std::uint8_t>((r + m) * 255.0f + 0.5f),
        static_cast<std::uint8_t>((g + m) * 255.0f + 0.5f),
        static_cast<std::uint8_t>((b + m) * 255.0f + 0.5f),
        255
    };
}

// ––––––––[ rgbToHsv ] RGB → HSV (h: 0-360, s: 0-1, v: 0-1)
inline void rgbToHsv(Color c, float& h, float& s, float& v) noexcept {
    const float r = c.r / 255.0f;
    const float g = c.g / 255.0f;
    const float b = c.b / 255.0f;
    const float mx = std::max({r, g, b});
    const float mn = std::min({r, g, b});
    const float df = mx - mn;
    v = mx;
    s = (mx == 0.0f) ? 0.0f : df / mx;
    if (df == 0.0f) {
        h = 0.0f;
    } else {
        if (mx == r) h = 60.0f * (0.0f + (g - b) / df);
        else if (mx == g) h = 60.0f * (2.0f + (b - r) / df);
        else h = 60.0f * (4.0f + (r - g) / df);
        if (h < 0.0f) h += 360.0f;
    }
}

namespace detail {
inline std::uint8_t hex_nibble(char c) noexcept {
    if (c >= '0' && c <= '9') return static_cast<std::uint8_t>(c - '0');
    if (c >= 'a' && c <= 'f') return static_cast<std::uint8_t>(c - 'a' + 10);
    if (c >= 'A' && c <= 'F') return static_cast<std::uint8_t>(c - 'A' + 10);
    return 0xFFu;  
}
struct NamedColor { std::string_view name; std::uint32_t argb; };
constexpr NamedColor kNamedColors[] = {
    {"transparent", 0x00000000u},
    {"black",       0xFF000000u},
    {"white",       0xFFFFFFFFu},
    {"red",         0xFFFF0000u},
    {"green",       0xFF00FF00u},
    {"blue",        0xFF0000FFu},
    {"gray",        0xFF808080u}, {"grey",        0xFF808080u},
    {"yellow",      0xFFFFFF00u},
    {"cyan",        0xFF00FFFFu},
    {"magenta",     0xFFFF00FFu},
    {"accent",      0xFF4F8DFFu},  
};
}  

constexpr Rect box(float l, float t, float r, float b) noexcept {
    return Rect{l, t, r - l, b - t};
}

inline std::optional<Color> Color::fromString(std::string_view s) noexcept {
    if (s.empty()) return std::nullopt;
    if (s.front() == '#') {
        const auto hex = s.substr(1);
        const auto len = hex.size();
        auto byte = [](std::string_view h, std::size_t i) -> std::uint8_t {
            return static_cast<std::uint8_t>(
                (detail::hex_nibble(h[i])     << 4) |
                 detail::hex_nibble(h[i + 1]));
        };
        auto nib = [](std::string_view h, std::size_t i) -> std::uint8_t {
            const auto n = detail::hex_nibble(h[i]);
            return (n == 0xFFu) ? std::uint8_t{0} :
                   static_cast<std::uint8_t>(n | (n << 4));
        };
        if (len == 3) {
            return Color{nib(hex, 0), nib(hex, 1), nib(hex, 2), 255};
        }
        if (len == 4) {
            return Color{nib(hex, 0), nib(hex, 1), nib(hex, 2), nib(hex, 3)};
        }
        if (len == 6) {
            return Color{byte(hex, 0), byte(hex, 2), byte(hex, 4), 255};
        }
        if (len == 8) {
            return Color{byte(hex, 2), byte(hex, 4), byte(hex, 6), byte(hex, 0)};
        }
        return std::nullopt;
    }
    for (const auto& nc : detail::kNamedColors) {
        if (nc.name == s) return Color::fromHex(nc.argb);
    }
    return std::nullopt;
}
}  

