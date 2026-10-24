# Tutorial 5 — Icons and Themes

Add Lucide icons to your text and let the user switch between a dark and a light theme at runtime.

## What you will build

A window with an icon-and-text label, an icon-and-text button, and a button that toggles the theme. Each click swaps the whole color scheme.

## What you will learn

- How to use `Inline` and `$()` to mix icons with text.
- How to switch themes at runtime.
- How `Color::fromString` can set a custom accent color.

## The words you need

- **Icon** — a small built-in drawing. Fluffy includes the Lucide icon set, and you name an icon with a value such as `fluffy::Icons::rocket`.
- **`Inline`** — a piece of content made of one or more runs of text and icons. A label or a button can display it.
- **`$()`** — a shorthand that builds an `Inline` stream. Each `<<` adds another icon or piece of text to the stream.
- **Theme** — a bundle of colors and measurements that controls how the whole window looks.
- **Light and dark** — two ready-made themes, `Theme::defaultLight()` and `Theme::defaultDark()`. A new window starts with the light one.

The [Inline](../inline.md), [icons](../icons.md), and [theme](../theme.md) references go into more detail.

## The full program

```cpp
#include <fluffy/button.hpp>
#include <fluffy/inline.hpp>
#include <fluffy/label.hpp>
#include <fluffy/window.hpp>
#include <fluffy/icons.hpp>
#include <fluffy/theme.hpp>

int main() {
    fluffy::Window win = fluffy::Window::open("Icons & Themes", 640, 400);
    if (!win) return 1;

    auto& label = win.add<fluffy::Label>(
        fluffy::$(fluffy::Icons::rocket << " Ready for launch"));
    label.setBounds(fluffy::Rect{40, 40, 560, 32});

    auto& themeBtn = win.add<fluffy::Button>(
        fluffy::$(fluffy::Icons::moon << " Toggle theme"));
    themeBtn.setBounds(fluffy::Rect{40, 120, 160, 40});

    bool dark = false;
    themeBtn.onClick([&] {
        dark = !dark;
        win.setTheme(dark ? fluffy::Theme::defaultDark()
                          : fluffy::Theme::defaultLight());
    });

    win.run();
}
```

## Build and run

Compile this program the same way you compiled the first tutorial, then run the resulting executable. If you need the build commands, revisit [Tutorial 1](01-hello-fluffy.md#build-and-run).

## Step by step

### 1. Include the headers you use

```cpp
#include <fluffy/button.hpp>
#include <fluffy/inline.hpp>
#include <fluffy/label.hpp>
#include <fluffy/window.hpp>
#include <fluffy/icons.hpp>
#include <fluffy/theme.hpp>
```

Including `<fluffy/icons.hpp>` makes the icon names available. The other headers bring in the widgets, the `Inline` type, and the theme types used by this program.

### 2. Mix an icon and text

```cpp
fluffy::$(fluffy::Icons::rocket << " Ready for launch")
```

`$()` builds an `Inline` stream, and `<<` adds pieces to it. Here the stream holds the rocket icon followed by the words " Ready for launch". The leading space in the string keeps the icon and the words from touching.

The same expression works as the text argument for a `Label` or a `Button`, as shown in the full program. Both widgets accept either plain text or `Inline` content.

### 3. Switch the theme

```cpp
bool dark = false;
themeBtn.onClick([&] {
    dark = !dark;
    win.setTheme(dark ? fluffy::Theme::defaultDark()
                      : fluffy::Theme::defaultLight());
});
```

`setTheme` swaps the entire color scheme, and the next frame uses the new theme. The `dark` variable remembers which theme is active, just as `count` remembered the number in [Tutorial 2](02-counter.md). The `[&]` callback captures it by reference so each click can flip it.

A new window starts with `Theme::defaultLight()`, so `dark` starts as `false`. The first click changes it to `true` and applies the dark theme; the next click changes it back.

### 4. Try a custom accent color

A `Color` is an RGBA color value. `Color::fromString` reads a hex string such as `"#4F8DFF"`. It returns an optional color, so check it before using it:

```cpp
fluffy::Theme custom = fluffy::Theme::defaultLight();
if (auto accent = fluffy::Color::fromString("#4F8DFF")) {
    custom.colors.accent = *accent;
}
win.setTheme(custom);
```

Starting from a built-in theme and changing one field keeps the rest of the theme intact while giving the controls a new accent color.

## Exercises

1. Add a second button with a different icon, such as `fluffy::Icons::check`.
2. Build a custom theme by copying `Theme::defaultLight()` and changing `colors.accent`.
3. Use `Color::fromString("#4F8DFF")` to set that accent color.

## Next tutorial

[YAML UI →](06-yaml-ui.md)
