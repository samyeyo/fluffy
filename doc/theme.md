# Theme

A **theme** is the set of visual choices Fluffy uses to draw your app: colors, spacing, text size, fonts, animation speeds, and shadows. Set a theme on a window and every widget in it follows that theme. Change the theme and the whole window changes at once — you do not restyle widgets one by one.

## What it's for

Use a theme when you want to control how your interface looks without changing the widgets themselves. A theme groups colors, layout metrics, animation timings, and shadows. Fluffy ships with three ready-made themes, called presets, and you can also start from a preset and change individual fields. A new window starts with `Theme::defaultLight()`.

## The words you need

- **Window** — the operating-system window your app draws into. `Window::open` creates one.
- **Widget** — any visible element inside a window, such as a label or a button. `win.add<T>(...)` adds one and returns a reference to it.
- **Theme** — the bundle of colors, metrics, fonts, and animation settings used to draw a window.
- **Preset** — a complete theme that Fluffy provides for you. You can use it as-is or copy it and change a few fields.
- **Color** — an RGBA color value. `Color::fromHex` builds one from a packed `0xAARRGGBB` number.
- **Metric** — a number that controls size or spacing, such as `padding`, `corner_radius`, or `text_size`.
- **Callback** — a small function you give to a widget so it can call you back when something happens, such as a click. In C++ you usually write these as lambdas, like `[] { … }`.

## A first example

This program opens a window, adds a label and a button, and lets the button switch between the light and dark presets while the app runs.

```cpp
#include <fluffy/fluffy.hpp>

int main() {
    fluffy::Window win = fluffy::Window::open("Theme demo", 640, 400);
    if (!win) return 1;

    auto& label = win.add<fluffy::Label>("Theme demo");
    label.setBounds(fluffy::Rect{40, 40, 560, 32});

    bool dark = false;
    auto& btn = win.add<fluffy::Button>("Switch theme");
    btn.setBounds(fluffy::Rect{40, 120, 160, 40});
    btn.onClick([&] {
        dark = !dark;
        win.setTheme(dark ? fluffy::Theme::defaultDark()
                          : fluffy::Theme::defaultLight());
    });

    win.run();
}
```

`Window::open` creates the window, and `if (!win)` checks that it opened successfully. `win.add<fluffy::Label>(...)` creates a label inside the window and returns a reference to it; `auto&` keeps that reference, so you work with the real widget rather than a copy. The window owns the widget for its whole life, so you never delete it. `setBounds` places a widget with a rectangle, written `Rect{x, y, width, height}`; `x` and `y` are the top-left corner, and `width` and `height` are the size in pixels.

`win.setTheme(...)` replaces the window's current theme. The next frame is drawn with the new colors and measurements, so every widget updates together. The `bool dark` variable remembers which theme is active, and the lambda passed to `onClick` flips it on each click.

The three presets are `fluffy::Theme::defaultLight()`, `fluffy::Theme::defaultDark()`, and `fluffy::Theme::solarized()`. You can call `setTheme` at startup or later, as this example does. The snippets on the rest of this page assume `win` is the window opened above.

## Common tasks

### Customize a preset

Start from a preset, change the fields you care about, then apply the result:

```cpp
fluffy::Theme theme = fluffy::Theme::defaultDark();
theme.colors.background = fluffy::Color::fromHex(0xFF101010);
theme.colors.accent = fluffy::Color::fromHex(0xFF4F8DFF);
theme.metrics.corner_radius = 12.0f;
theme.metrics.text_size = 16.0f;
win.setTheme(theme);
```

`defaultDark()` gives you a complete theme. Changing `colors.background` and `colors.accent` only touches those colors; everything else keeps the preset's values. `Color::fromHex` takes an `0xAARRGGBB` value, where the leading `FF` means fully opaque. `metrics.corner_radius` and `metrics.text_size` adjust the rounded corners and the default text size.

### Load a theme from YAML

A theme can also be written as YAML, a plain-text format that uses indentation to group related settings. This example sets a name, a font family, a few colors, two metrics, and one animation timing:

```yaml
name: "midnight"
font_family: "default"
colors:
  background: "#101010"
  accent: "#4F8DFF"
metrics:
  corner_radius: 12
  text_size: 16
animation:
  hover_tau: 0.12
```

Load the file and apply the result:

```cpp
fluffy::Theme theme = fluffy::Theme::fromYamlFile("theme.yaml");
win.setTheme(theme);
```

`Theme::fromYamlFile(path)` reads the file and returns a `Theme` you can pass to `setTheme`. If the YAML is already in memory, call `Theme::fromYaml(text)` instead. Keys you leave out keep their default values. If the file cannot be read, `fromYamlFile` falls back to `Theme::defaultLight()`.

The theme YAML recognizes `name`, `font_family`, `colors`, `metrics`, `shadows`, and `animation`. The Quick reference below lists the individual fields.

### Register a custom font

If you have loaded a font file into memory, register it with the window, then name it in the theme:

```cpp
win.registerFontMemory("Inter", ttf_data, ttf_data_size);
fluffy::Theme theme = fluffy::Theme::defaultLight();
theme.font_family = "Inter";
win.setTheme(theme);
```

Here `ttf_data` points to the font bytes and `ttf_data_size` is their length. `registerFontMemory` gives those bytes a name. Setting `theme.font_family` to the same name makes that font the default for every widget that does not choose its own.

### Show a slider value with a custom format

A `Slider` is a widget with a track and a draggable thumb. It can show its current value above the thumb, and you can choose how that value is formatted.

```cpp
auto& slider = win.add<fluffy::Slider>(
    fluffy::Rect{40, 200, 300, 32}, 0.5f);
slider.setShowValue(true);
slider.setValueFormat("{:.1f} units");
```

`win.add<fluffy::Slider>(...)` creates the slider and returns a reference to it, so `slider` is the real widget. The second constructor argument is the starting value, `0.5f`. `setShowValue(true)` turns on the value display, and `setValueFormat` sets its format. The format uses C++20 `std::format` syntax, and the default is `"{:.2f}"`.

With `"{:.1f} units"`, the value `0.5` is displayed as `0.5 units`. See the [Slider](slider.md) documentation for the full API.

## Good to know

- All animation fields live under `theme.metrics.animation`, for example
  `theme.metrics.animation.hover_tau`.
- Each theme carries four shadows under `metrics`: `shadow`, `shadow_hover`,
  `shadow_pressed`, and `shadow_disabled`. A `Shadow` has a `color`, a `blur`
  radius, and `offset_x` / `offset_y` offsets. A `blur` of `0` disables the
  shadow.
- A theme is a plain value. Copying a preset and changing a few fields leaves
  the original preset untouched, so you can keep several variants in one app.

## Quick reference

### Colors

| Field | Used for |
|---|---|
| `background` | Window background |
| `surface` | Widget backgrounds, cards |
| `on_surface` | Primary text and icons |
| `on_surface_mute` | Secondary text, placeholders |
| `accent` | Focus rings, hover highlights |
| `accent_pressed` | Pressed state |
| `border` | Borders and separators |
| `disabled` | Disabled text |

### Metrics

| Field | Meaning |
|---|---|
| `padding` | Default spacing between widgets |
| `corner_radius` | Rounded corner radius |
| `text_size` | Default text size |
| `border_width` | Default border thickness |
| `button_padding_x/y` | Button internal padding |
| `input_padding_x/y` | Text input internal padding |
| `input_min_height` | Minimum input height |
| `input_focus_width` | Focus ring thickness |
| `toggle_size` | Switch / checkbox indicator size |
| `slider_thumb_size` | Slider thumb diameter |
| `slider_track_height` | Slider track thickness |

### Animation

| Field | Meaning |
|---|---|
| `hover_tau` | How fast hover state changes (lower = snappier) |
| `press_tau` | How fast press state changes |
| `press_inset` | Pressed-state inset distance |
| `press_drop` | Pressed-state drop distance |
| `icon_size_gain` | Icons drawn at `text_size * icon_size_gain` |
| `icon_padding` | Padding around icons |

### Shadows

| Field | Meaning |
|---|---|
| `color` | Shadow color |
| `blur` | Blur radius (`0` disables the shadow) |
| `offset_x` / `offset_y` | Shadow offset |

### Theme identity and loading

| Field / Function | Meaning |
|---|---|
| `name` | Theme name (e.g. `"fluffy-light"`) |
| `font_family` | Default font spec for all widgets |
| `Theme::fromYaml(text)` | Parse a theme from a YAML string |
| `Theme::fromYamlFile(path)` | Parse a theme from a YAML file |

### Slider value format

Format specifiers follow C++20 `std::format` syntax:

| Format | Example output |
|---|---|
| `"{:.2f}"` | `0.50` |
| `"{:.1f} units"` | `0.5 units` |
| `"{:.0f}%"` | `50%` |
| `"{:>6.2f}"` | `  0.50` |

## See also

- [Window](window.md) — `setTheme` and `registerFontMemory`.
- [Geometry & colors](geom.md) — `Color` helpers.
- [Inline](inline.md) — icon sizing via the theme.
- [Slider](slider.md) — full slider API, including value display.
