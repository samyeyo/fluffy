# Inline — text + icons + colors

An **Inline** is a small stream of content that can mix text, icons, and colors in one value. Labels and buttons accept either a plain string or an Inline; use an Inline when the content is more than plain text.

## What it's for

Use an Inline when you want to compose rich content from several pieces instead of passing a single string. You build the stream by appending runs, then give it to a widget that accepts rich content. The widget stores the runs and draws them from left to right.

This is what mixed text and icon content means in Fluffy: one label or button can hold a small picture and words together, and the words can change color partway through.

## The words you need

- **Window** — the operating-system window your app draws into. `Window::open` creates one.
- **Widget** — any visible element inside a window, such as a label or a button. `win.add<T>(...)` adds one and returns a reference to it.
- **Inline** — one value that holds a sequence of content runs. A label or button can display it.
- **Run** — one piece of an Inline: a piece of text or an icon, plus any style that applies to it.
- **Icon** — a small built-in drawing. Fluffy includes the Lucide icon set, and you name an icon with a value such as `fluffy::Icons::rocket`. The [Icons](icons.md) page covers the set.
- **`$()`** — a shorthand macro that builds an Inline stream. `fluffy::$(fluffy::Icons::rocket << " Launch")` means `fluffy::Inline{} << fluffy::Icons::rocket << " Launch"`.
- **Color** — an RGBA color value. A color run changes the foreground color of the runs that follow it.
- **Style** — an optional combination of color, size, bold, and italic that applies to the runs that follow it.

## A first example

This program opens a window and adds a button whose content is a rocket icon followed by the word "Launch".

```cpp
#include <fluffy/fluffy.hpp>

int main() {
    fluffy::Window win = fluffy::Window::open("Inline demo", 640, 400);
    if (!win) return 1;

    auto& btn = win.add<fluffy::Button>(
        fluffy::$(fluffy::Icons::rocket << " Launch"));
    btn.setBounds(fluffy::Rect{40, 40, 160, 40});

    win.run();
}
```

`Window::open` creates the window, and `if (!win)` checks that it opened successfully. `win.add<fluffy::Button>(...)` creates a button inside the window and returns a reference to it; `auto&` keeps that reference, so `btn` is the real widget. `setBounds` places a widget with a rectangle, written `Rect{x, y, width, height}`; `x` and `y` are the top-left corner, and `width` and `height` are the size in pixels.

The button's content is built with `fluffy::$(...)`. Each `<<` adds another run to the stream: first the rocket icon, then the text `" Launch"`. The leading space keeps the icon and the word from touching. You can keep chaining `<<` to add more runs.

The `$` macro itself lives at global scope. This guide writes `fluffy::$(...)` so it is clear which type you get. If you add `using namespace fluffy;`, you can write `$(...)` instead. Including `<fluffy/inline.hpp>` makes the macro and the `Inline` type available; `<fluffy/fluffy.hpp>` includes it for you.

The snippets on the rest of this page assume `win` is the window opened above.

## Common tasks

### Add an icon to a button

You can create a button first and set its rich content later:

```cpp
auto& btn = win.add<fluffy::Button>("Done");
btn.setBounds(fluffy::Rect{40, 100, 140, 40});
btn.setInline(fluffy::$(fluffy::Icons::check << " Done"));
```

`setInline` replaces the button's plain label with the Inline stream. The `auto&` keeps `btn` as a reference to the widget that the window owns.

Call `setBounds` only when you position the button by hand; layouts assign geometry for you.

### Color text

```cpp
fluffy::$(fluffy::Color::fromHex(0xFF4F8DFF)
         << "Blue "
         << fluffy::Color::fromHex(0xFFEFEFEF)
         << "text");
```

A `Color` run sets the foreground color for the runs that follow it, like switching the pen color. The first color applies to `"Blue "`, and the second color applies to `"text"`. `Color::fromHex` takes a packed `0xAARRGGBB` value, where `FF` means fully opaque.

### Combine several runs

```cpp
fluffy::$(fluffy::Icons::star
         << " "
         << "4.9 / 5");
```

Here the stream contains an icon run, a space, and a rating string. Runs are drawn in the order you add them.

### Use a plain string where an Inline is expected

Widgets that show rich content also have a plain-string path. `Label` accepts a string in its constructor or through `setText`, and rich content through `setInline` or `setContent`. `Button` does the same with `setLabel` and `setInline` (or `setText`, which takes an `Inline`). `ComboBox::setText` also takes an `Inline`.

When you already have a plain string but need an `Inline`, build one explicitly with `fluffy::Inline::text("...")` or `fluffy::$("...")`.

### Use an Inline in a label

```cpp
auto& label = win.add<fluffy::Label>(
    fluffy::$(fluffy::Icons::rocket << " "
             << fluffy::Color::fromHex(0xFFEFEFEF)
             << "Ready for launch"));
label.setBounds(fluffy::Rect{40, 40, 400, 32});
label.setHorizontal(fluffy::Label::Align::Left);
```

This label combines an icon, a space, a color change, and text. It is positioned explicitly and aligned to the left. The `auto&` makes `label` a reference to the widget created inside the window.

### Inspect the runs

```cpp
fluffy::Inline inl = fluffy::$(fluffy::Icons::star << " 4.9 / 5");
const auto& runs = inl.runs();
bool empty = inl.empty();
```

`runs()` returns a `vector<Inline::Run>`. Each `Run` carries `type` (`Text` / `SvgIcon`), `utf8`, `svg_icon`, plus optional `color`, `size`, `bold` and `italic`. `empty()` tells you whether the stream contains anything.

Widgets handle the drawing for you. You normally read the runs only when you are building a custom widget or checking what content an Inline holds.

### Where it works

- `Label` constructor or `setInline`
- `Button` constructor or `setInline`
- `TextInput::setPrefix`
- Dialog message and button arguments

## Good to know

- The `$` macro is defined at global scope: write `fluffy::$(...)` for `fluffy::Inline`, or add `using namespace fluffy;` and write `$(...)`.
- Each `<<` adds one run. A `Color` or `Style` changes the runs that come after it, not the ones already added.
- Plain strings still work directly in widget constructors and in the plain-text setters. You only need an `Inline` when you want to mix content.

## Quick reference

| Token | Effect |
|---|---|
| `"text"` | Text run |
| `fluffy::Icons::name` | Icon run |
| `fluffy::Color` | Sets foreground color for following runs |
| `fluffy::Style{fg, size, bold, italic}` | Sets color, size, bold and italic for following runs (`fg` is `std::optional<Color>` — empty clears the color) |
| `fluffy::ResetStyle{}` | Reverts to inherited defaults |

## See also

- [Icons](icons.md) — the icon catalog.
- [Label](label.md) · [Button](button.md) · [TextInput](textinput.md).
- [Theme](theme.md) — `icon_size_gain`, `icon_padding`.
