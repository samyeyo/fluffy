# Geometry & colors

Fluffy uses two small helper types for positioning and coloring — `Rect` and `Color` — and both are plain data you can copy easily.

## What it's for

A **bound** is where a widget sits and how big it is, and in Fluffy a bound is a `Rect`. You pass a `Rect` to a widget to place it, and a `Color` to say how something looks. Widgets, themes, and drawing code all take these types.

Coordinates start at the top-left of the window and increase to the right and downward:

```text
(0,0) ──────────→ x
  │
  ↓
  y
```

This page also covers `Result<T>`: a small box that holds either a value or an error message. Many Fluffy factories return one when an operation can fail.

## A first example

```cpp
fluffy::Rect box = fluffy::Rect{40, 24, 320, 32};
auto c = fluffy::Color::fromString("#efefef");

if (c) {
    // box is at x = 40, y = 24, 320 wide and 32 high
    // *c is a light gray color
}
```

The rectangle takes four values in the order `x`, `y`, `width`, `height`. `Color::fromString` reads a hex string and returns a `std::optional<Color>` — a value that may or may not be there — so you check it with `if (c)` before using `*c`.

## Common tasks

### Make a rectangle

You can build a `Rect` directly from a position and a size:

```cpp
// x, y, width, height
fluffy::Rect box = fluffy::Rect{40, 24, 320, 32};
```

When you know the edges instead of the dimensions, use the `box()` helper:

```cpp
// left, top, right, bottom — using the box() helper
fluffy::Rect box2 = fluffy::box(40, 24, 360, 56);
//                            width = 360 - 40 = 320
//                            height = 56 - 24 = 32
```

`box(l, t, r, b)` converts corner coordinates to the `Rect{x, y, w, h}` form.

### Read and test a rectangle

A `Rect` can answer a few questions about itself:

```cpp
box.left();     // x
box.top();      // y
box.right();    // x + width
box.bottom();   // y + height
box.width();    // w
box.height();   // h
box.center_x(); // x + w / 2
box.center_y(); // y + h / 2
box.empty();    // true if w <= 0 or h <= 0

box.containsPoint(100, 50);  // true if inside
```

You rarely change a `Rect` directly. Usually you pass it to `win.add<MyWidget>(...)` or set it with `widget.setBounds(rect)`, and the layout or widget updates its own bounds.

### Build a color

A color is four bytes: red, green, blue, alpha.

```cpp
fluffy::Color red   {255, 0,   0,   255}; // opaque red
fluffy::Color glass {255, 255, 255, 128}; // half-transparent white
```

The easiest way in practice is to start from a hex string:

```cpp
auto c1 = fluffy::Color::fromString("#ff0000");     // red
auto c2 = fluffy::Color::fromString("#efefef");     // light gray
auto c3 = fluffy::Color::fromString("#80ff0000");   // transparent red (alpha first)
```

Eight-digit strings use the `#aarrggbb` form, with alpha first, matching `fromHex`. The supported forms are:

| Form | Example | Alpha |
|---|---|---|
| `#rgb` | `#f00` | opaque |
| `#rgba` | `#f008` | 4th digit (`#rgba` order) |
| `#rrggbb` | `#ff0000` | opaque |
| `#aarrggbb` | `#80ff0000` | first two digits |

Because `fromString` returns a `std::optional<Color>`, check the result when the string comes from user input:

```cpp
auto c = fluffy::Color::fromString("#bad-color");
if (c) { /* use *c */ }
```

You can also use named colors:

```cpp
auto c = fluffy::Color::fromString("red");      // named
auto a = fluffy::Color::fromString("accent");   // theme brand color
```

The available names are `transparent`, `black`, `white`, `red`, `green`, `blue`, `gray`, `grey`, `yellow`, `cyan`, `magenta`, and `accent`.

For a packed hex integer, integers are ARGB (`0xAARRGGBB`):

```cpp
auto c = fluffy::Color::fromHex(0xFFFF0000); // opaque red
```

### Change alpha and blend colors

```cpp
auto faded = color.withAlpha(128);              // uint8_t 0-255
auto mid = fluffy::mix(a, b, 0.5f);             // Color or float lerp
auto tri = fluffy::blend3(a, b, c, 0.5f, 0.5f); // mix(mix(a, b, t1), c, t2)
auto rgb = fluffy::hsvToRgb(210.0f, 0.5f, 1.0f);
fluffy::rgbToHsv(color, h, s, v);
```

### Handle a fallible call with `Result`

Some Fluffy calls can fail instead of returning a usable value. Rather than throwing an exception, they return a `Result<T>` — a small box that holds either a value of type `T` or an error message. You check it before use:

```cpp
fluffy::Result<int> r = fluffy::Result<int>::err("something went wrong");
if (!r) {
    std::printf("Error: %s\n", r.error().c_str());
    return 1;
}
int value = r.value(); // only reached on success
```

`if (!r)` is true when the call failed. On success, `r.value()` (or `*r`) gives you the value; on failure, `r.error()` gives you the message. You can build a successful `Result` directly from a value, and a failed one with `Result<T>::err(message)`. `NoneResult` is the same idea for operations that succeed with no value at all, such as loading a UI file.

If a failure leaves you with nothing sensible to do, `r.unwrap()` returns the value on success and otherwise prints the error and stops the program. Prefer `if (!r)` in normal code, and reach for `unwrap()` only when there is truly no way to continue. The [Window](window.md) page shows both styles.

## Good to know

- Eight-digit color strings are `#aarrggbb`, with alpha first. This matches `fromHex`.
- `Color::fromHex` takes integers in `0xAARRGGBB` form.
- `Color::fromString` returns `std::optional<Color>`, so check it before using a value that may come from user input.
- `Rect::empty()` is true when `width <= 0` or `height <= 0`.

## Quick reference

```cpp
fluffy::Rect box = fluffy::Rect{40, 24, 320, 32};
fluffy::Rect box2 = fluffy::box(40, 24, 360, 56);

box.left();
box.top();
box.right();
box.bottom();
box.width();
box.height();
box.center_x();
box.center_y();
box.empty();
box.containsPoint(100, 50);
```

```cpp
auto c1 = fluffy::Color::fromString("#ff0000");
auto c2 = fluffy::Color::fromString("#efefef");
auto c3 = fluffy::Color::fromString("#80ff0000");
auto c4 = fluffy::Color::fromString("red");
auto c5 = fluffy::Color::fromString("accent");
auto c6 = fluffy::Color::fromHex(0xFFFF0000);

auto faded = color.withAlpha(128);
auto mid = fluffy::mix(a, b, 0.5f);
auto tri = fluffy::blend3(a, b, c, 0.5f, 0.5f);
auto rgb = fluffy::hsvToRgb(210.0f, 0.5f, 1.0f);
fluffy::rgbToHsv(color, h, s, v);
```

| Form | Example | Alpha |
|---|---|---|
| `#rgb` | `#f00` | opaque |
| `#rgba` | `#f008` | 4th digit (`#rgba` order) |
| `#rrggbb` | `#ff0000` | opaque |
| `#aarrggbb` | `#80ff0000` | first two digits |

## See also

- [Window](window.md) — uses `Rect` for widget positions and `Color` for themes.
- [Theme](theme.md) — `Color` in action.
