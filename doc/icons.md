# Icons

Fluffy includes 654 built-in SVG icons from the [Lucide](https://lucide.dev) set. There are no image files to ship or load: each icon has a name under the `fluffy::Icons` namespace, such as `fluffy::Icons::rocket`, and you use it inside an [Inline](inline.md) stream with text.

## What it's for

Use these icons when a button, label, or other rich-text widget needs a recognizable glyph. An icon value is a lightweight handle to the built-in SVG data, and you can draw one on its own with `fluffy::Icons::renderIcon(...)` if you are customizing a widget.

## The words you need

- **Icon** — a small built-in drawing. You name one with a value such as `fluffy::Icons::rocket`.
- **Inline** — one value that mixes text, icons, and colors. Labels and buttons can display it.
- **`$()`** — a shorthand macro that builds an Inline stream, so an icon and text can sit side by side.
- **`SvgIconRef`** — the small value an icon name refers to. When you use an icon inside an Inline, you never handle this type directly.
- **Theme** — the bundle of colors and measurements that controls how icons are sized and colored.

## A first example

This program opens a window and adds a button that contains the `fluffy::Icons::rocket` icon followed by the text `" Launch"`.

```cpp
#include <fluffy/fluffy.hpp>

int main() {
    fluffy::Window win = fluffy::Window::open("Icons", 640, 400);
    if (!win) return 1;

    auto& btn = win.add<fluffy::Button>(
        fluffy::$(fluffy::Icons::rocket << " Launch"));
    btn.setBounds(fluffy::Rect{40, 40, 160, 40});

    win.run();
}
```

`Window::open` creates the window, and `if (!win)` checks that it opened successfully. `win.add<fluffy::Button>(...)` creates a button inside the window and returns a reference to it; `auto&` keeps that reference, so `btn` is the real widget. `setBounds` places a widget with a rectangle, written `Rect{x, y, width, height}`; `x` and `y` are the top-left corner, and `width` and `height` are the size in pixels.

The icon is streamed into the button through the `$` macro: `fluffy::$(fluffy::Icons::rocket << " Launch")` builds an Inline containing the rocket icon and the text that follows. Icons take the surrounding text color, and the theme controls their size. The snippets on the rest of this page assume `win` is the window opened above.

## Common tasks

### Add an icon to a button

Create a button first, then set its rich content with `setInline`:

```cpp
auto& btn = win.add<fluffy::Button>("Launch");
btn.setBounds(fluffy::Rect{40, 40, 160, 40});
btn.setInline(fluffy::$(fluffy::Icons::rocket << " Launch"));
```

`setInline` replaces the button's plain label with the icon-and-text content. `auto&` makes `btn` a reference to the widget created inside the window.

### Look up the right icon name

Lucide names use kebab-case. In Fluffy they become snake_case. For example, the Lucide name `arrow-down` becomes `fluffy::Icons::arrow_down`. The name rules below show more examples. If you have the name as a string, `fluffy::Icons::find("arrow_down")` returns a pointer to the icon, or `nullptr` when the name is unknown.

### Render an icon directly

If you are drawing your own widget and do not need an Inline, you can draw an icon through its lower-level call:

```text
fluffy::Icons::renderIcon(vg, icon, x, y, size, color)
```

`vg` is the NanoVG drawing context you get from `win.nvgContext()`, `icon` is a value such as `fluffy::Icons::rocket`, and `color` is an `NVGcolor`. Most apps never need this; an Inline is easier.

### Browse the full catalog

The icon showcase example in the repository displays every icon in a grid, which makes it easy to pick a name. In code, include `<fluffy/icons.hpp>` and browse the names offered by `fluffy::Icons::` in your editor.

## Good to know

- If a Lucide name conflicts with a C++ keyword, use an alternative icon.
- Icons inherit the color of the text around them. The theme fields `icon_size_gain` and `icon_padding` control their size and spacing.

## Quick reference

### Common icons

| Use | Icon |
|---|---|
| Save | `fluffy::Icons::save` |
| Delete | `fluffy::Icons::trash` |
| Edit | `fluffy::Icons::edit` |
| Search | `fluffy::Icons::search` |
| Settings | `fluffy::Icons::cog` |
| Home | `fluffy::Icons::home` |
| User | `fluffy::Icons::user` |
| Check | `fluffy::Icons::check` |
| Warning | `fluffy::Icons::alert_triangle` |
| Close | `fluffy::Icons::x` |

### Icon name rules

| Lucide | Fluffy |
|---|---|
| `arrow-down` | `fluffy::Icons::arrow_down` |
| `chevrons-up-down` | `fluffy::Icons::chevrons_up_down` |
| `file-text` | `fluffy::Icons::file_text` |

In YAML `inline:` runs, use the underscore name without the `fluffy::Icons::`
prefix: `icon: arrow_down`.

## See also

- [Inline](inline.md) — how to mix icons and text.
- [Theme](theme.md) — `icon_size_gain`, `icon_padding`.
- [Label](label.md) · [Button](button.md) · [TextInput](textinput.md).
