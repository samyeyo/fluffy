// src/theme.cpp

// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

// Theme — defaultLight / defaultDark / solarized factory implementations.

#include <fluffy/theme.hpp>
#include <fluffy/yaml_fwd.hpp>
#include <fstream>
#include <sstream>
#include <string>
namespace fluffy {
namespace {

// ––––––––[ readColor ] reads a hex color from a YAML child node.
void readColor(const YamlNode& parent, const char* key, Color& out) {
    auto n = parent[key];
    if (n.IsScalar()) {
        auto c = Color::fromString(n.asString());
        if (c) out = *c;
    }
}

// ––––––––[ readFloat ] reads a float metric from a YAML child node.
void readFloat(const YamlNode& parent, const char* key, float& out) {
    auto n = parent[key];
    if (n.IsScalar()) out = n.asFloat();
}

// ––––––––[ readShadow ] parses a Shadow struct from a YAML map node.
void readShadow(const YamlNode& parent, const char* key, Shadow& out) {
    auto n = parent[key];
    if (!n.IsMap()) return;
    readColor(n, "color", out.color);
    readFloat(n, "blur",     out.blur);
    readFloat(n, "offset_x", out.offset_x);
    readFloat(n, "offset_y", out.offset_y);
}

}  // namespace

// ––––––––[ fromYaml ] parse YAML theme definition.
Theme Theme::fromYaml(std::string_view yaml_content) {
    Theme t;
    std::string buf(yaml_content);
    dyml::Directyaml dy = parseYaml(buf);
    YamlNode doc(dy);
    if (!doc.IsMap()) return t;

    if (auto n = doc["name"]; n.IsScalar()) t.name = n.asString();
    if (auto f = doc["font_family"]; f.IsScalar()) t.font_family = f.asString();

    // Colors
    if (auto c = doc["colors"]; c.IsMap()) {
        readColor(c, "background",      t.colors.background);
        readColor(c, "surface",         t.colors.surface);
        readColor(c, "on_surface",      t.colors.on_surface);
        readColor(c, "on_surface_mute", t.colors.on_surface_mute);
        readColor(c, "accent",          t.colors.accent);
        readColor(c, "accent_pressed",  t.colors.accent_pressed);
        readColor(c, "border",          t.colors.border);
        readColor(c, "disabled",        t.colors.disabled);
    }

    // Metrics
    if (auto m = doc["metrics"]; m.IsMap()) {
        readFloat(m, "padding",           t.metrics.padding);
        readFloat(m, "corner_radius",     t.metrics.corner_radius);
        readFloat(m, "text_size",         t.metrics.text_size);
        readFloat(m, "border_width",      t.metrics.border_width);
        readFloat(m, "button_padding_x",  t.metrics.button_padding_x);
        readFloat(m, "button_padding_y",  t.metrics.button_padding_y);
        readFloat(m, "input_padding_x",   t.metrics.input_padding_x);
        readFloat(m, "input_padding_y",   t.metrics.input_padding_y);
        readFloat(m, "input_min_height",  t.metrics.input_min_height);
        readFloat(m, "input_focus_width", t.metrics.input_focus_width);
        readFloat(m, "toggle_size",          t.metrics.toggle_size);
        readFloat(m, "slider_track_height",  t.metrics.slider_track_height);
        readFloat(m, "slider_thumb_size",    t.metrics.slider_thumb_size);
    }

    // Shadows
    if (auto s = doc["shadows"]; s.IsMap()) {
        readShadow(s, "shadow",         t.metrics.shadow);
        readShadow(s, "shadow_hover",   t.metrics.shadow_hover);
        readShadow(s, "shadow_pressed", t.metrics.shadow_pressed);
        readShadow(s, "shadow_disabled", t.metrics.shadow_disabled);
    }

    // Animation
    if (auto a = doc["animation"]; a.IsMap()) {
        readFloat(a, "hover_tau",     t.metrics.animation.hover_tau);
        readFloat(a, "press_tau",     t.metrics.animation.press_tau);
        readFloat(a, "press_inset",   t.metrics.animation.press_inset);
        readFloat(a, "press_drop",    t.metrics.animation.press_drop);
        readFloat(a, "icon_size_gain", t.metrics.animation.icon_size_gain);
        readFloat(a, "icon_padding",  t.metrics.animation.icon_padding);
    }

    return t;
}

// ––––––––[ fromYamlFile ] load YAML theme from a file path.
Theme Theme::fromYamlFile(const std::string& filepath) {
    std::ifstream in(filepath);
    if (!in) {
        return defaultLight();
    }
    std::stringstream buf;
    buf << in.rdbuf();
    return fromYaml(buf.str());
}

Theme Theme::defaultLight() {
    Theme t;
    t.name = "fluffy-light";
    // WinUI 3 light — matches Common_themeresources_any.xaml (Light)
    // SolidBackgroundFillColorBase #F3F3F3, surface #FFFFFF,
    // TextFillColorPrimary #E4000000, secondary #9E000000,
    // SystemAccentColor #0078D4 / Dark1 #005A9E,
    // CardStrokeColorDefaultSolid #EBEBEB, TextFillColorDisabled #5C000000.
    t.colors.background      = Color::fromHex(0xFFF3F3F3);
    t.colors.surface         = Color::fromHex(0xFFFFFFFF);
    t.colors.on_surface      = Color::fromHex(0xE4000000);
    t.colors.on_surface_mute = Color::fromHex(0x9E000000);
    t.colors.accent          = Color::fromHex(0xFF0078D4);
    t.colors.accent_pressed  = Color::fromHex(0xFF005A9E);
    t.colors.border          = Color::fromHex(0xFFEBEBEB);
    t.colors.disabled        = Color::fromHex(0x5C000000);
    t.metrics.padding        = 10.0f;
    t.metrics.corner_radius  = 6.0f;
    t.metrics.border_width   = 1.0f;
    t.metrics.shadow = Shadow{
        Color::fromHex(0x0A000000), 14.0f, 0.0f, 2.0f
    };
    t.metrics.shadow_hover = Shadow{
        Color::fromHex(0x12000000), 18.0f, 0.0f, 3.0f
    };
    t.metrics.shadow_pressed = Shadow{
        Color::fromHex(0x0C000000), 7.0f,  0.0f, 1.0f
    };
    t.metrics.shadow_disabled = Shadow{};
    return t;
}
Theme Theme::defaultDark() {
    Theme t;
    t.name = "fluffy-dark";
    // WinUI 3 dark — matches Common_themeresources_any.xaml (Dark)
    // SolidBackgroundFillColorBase #202020, quarternary #2C2C2C,
    // TextFillColorPrimary #FFFFFF, secondary #C5FFFFFF,
    // SystemAccentColor #0078D4 / Dark1 #005A9E,
    // CardStrokeColorDefaultSolid #1C1C1C / ControlStroke #12FFFFFF → #2F2F2F,
    // TextFillColorDisabled #5DFFFFFF.
    t.colors.background      = Color::fromHex(0xFF202020);
    t.colors.surface         = Color::fromHex(0xFF2C2C2C);
    t.colors.on_surface      = Color::fromHex(0xFFFFFFFF);
    t.colors.on_surface_mute = Color::fromHex(0xC5FFFFFF);
    t.colors.accent          = Color::fromHex(0xFF0078D4);
    t.colors.accent_pressed  = Color::fromHex(0xFF005A9E);
    t.colors.border          = Color::fromHex(0xFF2F2F2F);
    t.colors.disabled        = Color::fromHex(0x5DFFFFFF);
    t.metrics = Theme::defaultLight().metrics;
    t.metrics.shadow = Shadow{
        Color::fromHex(0x20000000), 20.0f, 0.0f, 3.0f
    };
    t.metrics.shadow_hover = Shadow{
        Color::fromHex(0x2C000000), 26.0f, 0.0f, 4.0f
    };
    t.metrics.shadow_pressed = Shadow{
        Color::fromHex(0x1E000000), 10.0f, 0.0f, 2.0f
    };
    t.metrics.shadow_disabled = Shadow{};
    return t;
}
Theme Theme::solarized() {
    Theme t;
    t.name = "solarized";
    t.colors.background      = Color::fromHex(0xFFFFFCF5);
    t.colors.surface         = Color::fromHex(0xFFFFF8EE);
    t.colors.on_surface      = Color::fromHex(0xFF073642);
    t.colors.on_surface_mute = Color::fromHex(0xFF586E75);
    t.colors.accent          = Color::fromHex(0xFF268BD2);
    t.colors.accent_pressed  = Color::fromHex(0xFF005A8C);
    t.colors.border          = Color::fromHex(0xFFECE5D4);
    t.colors.disabled        = Color::fromHex(0xFFB0ADA5);
    t.metrics.padding        = 10.0f;
    t.metrics.corner_radius  = 8.0f;
    t.metrics.border_width   = 1.0f;
    t.metrics.shadow = Shadow{
        Color::fromHex(0x0A000000), 14.0f, 0.0f, 2.0f
    };
    t.metrics.shadow_hover = Shadow{
        Color::fromHex(0x12000000), 18.0f, 0.0f, 3.0f
    };
    t.metrics.shadow_pressed = Shadow{
        Color::fromHex(0x0C000000), 7.0f,  0.0f, 1.0f
    };
    t.metrics.shadow_disabled = Shadow{};
    t.metrics.animation = Animation{
        /* hover_tau */ 0.20f,
        /* press_tau */ 0.11f,
        /* press_inset */ 1.0f,
        /* press_drop  */ 1.0f,
        /* icon_size_gain */ 1.20f,
        /* icon_padding   */ 0.10f,
    };
    return t;
}
}  

