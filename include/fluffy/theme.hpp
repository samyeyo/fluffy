// include/fluffy/theme.hpp

// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

// Theme — colours, metrics, animation and typography configuration.

#pragma once
#include <fluffy/geom.hpp>
#include <string>
#include <string_view>
namespace fluffy {
struct ColorScheme {
    Color background      = Color::fromHex(0xFF101010);
    Color surface         = Color::fromHex(0xFF1E1E22);
    Color on_surface      = Color::fromHex(0xFFEFEFEF);
    Color on_surface_mute = Color::fromHex(0xFFB6B6BC);
    Color accent          = Color::fromHex(0xFF4F8DFF);
    Color accent_pressed  = Color::fromHex(0xFF2C66D8);
    Color border          = Color::fromHex(0xFF3A3A40);
    Color disabled        = Color::fromHex(0xFF555560);
};
struct Shadow {
    Color color       = Color::fromHex(0x00000000);
    float blur        = 0.0f;
    float offset_x    = 0.0f;
    float offset_y    = 0.0f;
};
struct Animation {
    float hover_tau    = 0.15f;
    float press_tau    = 0.08f;
    float press_inset  = 1.0f;
    float press_drop   = 1.0f;
    float icon_size_gain = 1.20f;
    float icon_padding = 0.10f;
};
struct Metrics {
    float padding           = 10.0f;
    float corner_radius     = 6.0f;
    float text_size         = 14.0f;
    float border_width      = 1.0f;
    float button_padding_x  = 14.0f;
    float button_padding_y  = 9.0f;
    float input_padding_x   = 8.0f;
    float input_padding_y   = 6.0f;
    float input_min_height  = 28.0f;
    float input_focus_width = 1.5f;
    float toggle_size          = 16.0f;
    float slider_track_height  = 4.0f;
    float slider_thumb_size    = 14.0f;
    Shadow shadow;
    Shadow shadow_hover;
    Shadow shadow_pressed;
    Shadow shadow_disabled;
    Animation animation;
};
struct Theme {
    ColorScheme  colors;
    Metrics      metrics;
    static Theme defaultLight();
    static Theme defaultDark();
    static Theme solarized  ();
    static Theme fromYaml(std::string_view yaml_content);
    static Theme fromYamlFile(const std::string& filepath);
    std::string  name        = "fluffy-light";
    std::string  font_family = "default";
};
}  

