# Custom widgets

Fluffy ships with buttons, labels, tables and so on, but a widget is just a C++
class. This page shows how to write your own, starting from the small
`SimpleWidget` base and then moving to the full `Widget` base when you need raw
drawing, animation or keyboard focus.

## The two starting points

| Base | You implement | Use it when |
|---|---|---|
| `SimpleWidget` | `paint(const Painter&)` and optionally `onPress(x, y)` | the widget paints itself with rectangles, rounded rectangles, rings and text |
| `Widget` | `draw(NVGcontext*, const Theme&, const Window&)` and `handleEvent(...)` | you need NanoVG paths, gradients, animation, focus, or YAML properties |

`SimpleWidget` derives from `Widget` and fills in the plumbing: it wraps the
NanoVG context in a small `Painter`, routes mouse presses to `onPress`, and
forwards `naturalWidth`/`naturalHeight` to `preferredWidth`/`preferredHeight`.
You can still override any `Widget` hook (`updateHover`, `tick`, `setProperty`,
and so on) if the simple base is almost enough.

Both bases give you the same ownership, layout, event and rendering rules.

## What a NanoVG context is

NanoVG is a small vector drawing library that runs on top of OpenGL. It draws
paths, text and images with an API similar to the HTML canvas. It is an
*immediate-mode* library: nothing is retained between frames. Every frame your
`draw` emits the paths again, and NanoVG turns them into GPU work.

An `NVGcontext` is NanoVG's state object. It owns:

- the current path and the current fill/stroke paint,
- the active font face, font size and text alignment,
- the transform stack (translate, scale, rotate),
- the scissor (clip) rectangle and the global alpha,
- the font atlas and the image handles you created.

Fluffy creates one `NVGcontext` per window when the window is created
(OpenGL 3 on desktop, GLES 2 in the browser). It lives as long as the window and
is available through `Window::nvgContext()`. Do not create or destroy contexts
yourself, and do not call `nvgBeginFrame`/`nvgEndFrame` from a widget: the window
already brackets every frame.

### Coordinates

NanoVG draws in the window's *design* coordinates: the logical size you passed to
`Window::open` or `Window::Config`. The origin is the top-left corner, `x` grows to
the right and `y` grows downward. The window scales the whole frame for HiDPI
screens, so a widget draws the same numbers on a Retina display and on a normal
one. Use `bounds()` values directly; never multiply by a pixel ratio yourself.

### State is sticky

The context remembers its state between draw calls. If one widget sets a red fill
and a 3 px stroke, the next widget inherits them unless it sets its own. Always
set the font, fill, stroke and alignment you need inside `draw`. When you change
the transform or the scissor, wrap the change in `nvgSave(vg)` /
`nvgRestore(vg)` so you do not leak it to the widgets drawn after you.

### Fonts

Each window registers two faces at creation: `"default"` and `"default-bold"`.
`Painter::text` uses `"default"`. With raw NanoVG you select a face with
`nvgFontFace(vg, "default")` or `"default-bold"`. To use another family, register
it first (for example with `registerFontMemory`) and resolve its face name with
`prepareFont`.

## The life of a widget

A widget is owned by the window or by a container inside it:

```cpp
auto& badge = window.add<Badge>("Hello");        // owned by the window
auto& meter = column.add<LevelMeter>(Rect{});    // owned by the container
```

`add` constructs the widget with the arguments you pass, takes ownership through a
`std::unique_ptr`, and returns a reference. That reference stays valid until the
owning window is destroyed. A widget must not be copied.

Once per frame the window runs these steps, in this order:

1. `updateHover(mx, my)` — the cursor moved or the widget moved under it.
2. `tick(dt, theme)` — advance animations by the elapsed time.
3. `draw(vg, theme, window)` — paint the current state.

Input events (`handleEvent`) can arrive between frames at any time. Hover, tick
and events run on the UI thread; keep them short and never block.

`visible()` and `enabled()` are handled for you for hover and tick, but `draw`
and `handleEvent` should still check them if your widget can be hidden or
disabled.

## Geometry and layout

A widget has a `Rect{x, y, w, h}` in design coordinates, exposed by
`bounds()`, `width()` and `height()`. `setBounds()` sets it directly.

Layouts assign bounds automatically. When a widget's `autoSize()` is `Both`
(the default), a layout measures it with `naturalWidth()` and
`naturalHeight()`. The distributed axis uses that measurement, while the cross
axis usually follows the layout slot. A horizontal layout uses the natural
width; a vertical layout uses the natural height and keeps its column width.
`AutoSize::Width`, `AutoSize::Height` and `AutoSize::Disabled` opt out of one or
both axes. `SimpleWidget` maps those measurements to `preferredWidth` and
`preferredHeight`, which is why the examples below only override the simple
versions.

Other sizing concepts:

- **stretchWeight** — for children that are *not* auto-sized, a layout shares the
  leftover space in proportion to this weight (`setStretchWeight`).
- **fillsParent** — containers report `true` when they should cover their parent;
  the window uses it to size root containers on resize.
- **constraints** — anchors can pin, center or size a widget independently of the
  layout. Your widget does not need to know about them; the container solves them
  and writes the result into `bounds()`.

Because the container writes the bounds, create layout children with an empty
`Rect{}` and let the layout place them. Use `setBounds` only for widgets you place
manually.

## Events

Events are a `std::variant` declared in `<fluffy/event.hpp>`:

| Event | Fields |
|---|---|
| `MouseMoveEvent` | `x`, `y` |
| `MouseDownEvent` | `x`, `y`, `button` |
| `MouseUpEvent` | `x`, `y`, `button` |
| `MouseScrollEvent` | `dx`, `dy` |
| `KeyDownEvent` | `key`, `scancode`, `mods` |
| `CharEvent` | `codepoint` |
| `ResizeEvent` | `width`, `height` |
| `CloseEvent` | — |

Use `std::get_if<MouseDownEvent>(&event)` or the `kind_of(event)` helper. Mouse
coordinates are in design pixels, the same space as `bounds()`, so
`containsPoint(x, y)` works directly. `KeyDownEvent::key` is a GLFW key code.

Return `EventResult::Consumed` when you used the event and
`EventResult::Ignored` otherwise. The window walks widgets from the topmost one
down and stops at the first widget that consumes the event.

A click is a press followed by a release inside the widget. Track the press in a
member, consume both halves, and fire your action on the release:

```cpp
if (const auto* down = std::get_if<MouseDownEvent>(&event)) {
    if (down->button == MouseButton::Left && containsPoint(down->x, down->y)) {
        pressed_ = true;
        return EventResult::Consumed;
    }
}
if (const auto* up = std::get_if<MouseUpEvent>(&event)) {
    if (pressed_ && up->button == MouseButton::Left) {
        pressed_ = false;
        if (containsPoint(up->x, up->y)) fire();
        return EventResult::Consumed;
    }
}
```

### Focus and the keyboard

Keyboard events are delivered to every widget that is visible, so check whether
your widget has focus before acting on them. Advertise focusability with
`isFocusable()` and store the focus flag in `setFocused(bool)`. When the user
clicks the widget, call `window_->requestFocus(this)`.

The window handles Tab and Shift+Tab traversal, calls `setFocused` on the old and
new widgets, and exposes `focusNext()` / `focusPrevious()` if you need to move
focus yourself. `drawFocusRing(vg, bounds(), theme)` (from
`<fluffy/inline.hpp>`) draws the standard focus outline.

`KeyDownEvent::key` is the raw GLFW key code: Fluffy does not translate it to its
own enum. Include `<GLFW/glfw3.h>` and compare with the `GLFW_KEY_*` constants,
as the built-in widgets and examples do, or use the numeric values if you prefer
(`32` is Space, `257` is Enter, `256` is Escape). See the
[GLFW key codes](https://www.glfw.org/docs/latest/group__keys.html).

## Hover, the cursor and animation time

Hover is separate from mouse events. The window calls `updateHover(mx, my)` every
frame with the current cursor position, and starts each frame by resetting the
cursor to an arrow. If the pointer is over your widget, call
`window_->setCursorShape(CursorShape::Hand)` (or another shape) from
`updateHover`.

Animations use `SmoothAnim`, an exponentially decaying channel:

```cpp
SmoothAnim anim_;                 // current = 0, target = 0

anim_.setTarget(hovered_ ? 1.0f : 0.0f);   // choose the goal
anim_.tick(dt, tau);                       // advance once per frame
float t = anim_.value();                   // read in draw()
```

`tick` moves `current` toward `target_` with
`current += (target_ - current) * (1 - exp(-dt / tau))`. The result is
frame-rate independent and never overshoots. `tau` is the time constant: after
`tau` seconds the value has covered about 63 % of the remaining distance, and
after `3 * tau` about 95 %. Small values feel snappy, large values feel soft.
`snapTo(v)` jumps immediately, which is useful for initial state.

Where the time comes from:

- `dt` is the number of seconds since the previous frame, measured by the window.
- The window clamps `dt` to at most 0.05 s. After a stall or a breakpoint your
  animations advance at most one 50 ms step instead of jumping.
- `tick` runs after hover and before draw, once per visible and enabled widget.
  Containers forward it to their children.
- Use `theme.metrics.animation.hover_tau` (0.15 s) and `press_tau` (0.08 s) for
  hover and press feedback so custom widgets feel like the built-in ones.

Fluffy redraws every frame; there is no invalidate/repaint call and no manual
timer to start. Animate in `tick`, read the value in `draw`, and the window does
the rest. Do not compute time with `glfwGetTime()` inside `draw`, and do not call
`sleep`: that would make the UI stutter.

`SimpleWidget` inherits the same hooks. If a simple widget needs hover feedback,
override `updateHover` and `tick` on it exactly as you would on `Widget`.

## Theming

`draw` receives the active `Theme`. Use it instead of hard-coded colors:

- `theme.colors.background`, `surface`, `on_surface`, `on_surface_mute`
- `theme.colors.accent`, `accent_pressed`, `border`, `disabled`
- `theme.metrics.text_size`, `padding`, `corner_radius`, `border_width`
- `theme.metrics.animation.*`

`accentColor(theme)` returns the widget's own accent if `setAccentOverride` was
called, otherwise the theme accent; `accentPressedColor(theme)` does the same for
the pressed color. `mix(a, b, t)` blends two colors or two floats.

`Painter::text` uses the default face and the theme text size unless you pass a
size. For anything more elaborate, call NanoVG directly:
`nvgFontFace`, `nvgFontSize`, `nvgTextAlign`, `nvgText` and `nvgTextBounds`.

## Example 1 — a SimpleWidget badge

The badge draws a rounded pill, shows a click counter, and asks layouts for a
size. Two overrides are enough: `paint` and `onPress`.

```cpp
#include <cstdio>
#include <string>

#include <fluffy/fluffy.hpp>
#include <fluffy/simple_widget.hpp>

using namespace fluffy;

// ––––––––[ Badge ] rounded pill that counts its own clicks
class Badge final : public SimpleWidget {
public:
    explicit Badge(std::string text)
        : SimpleWidget(Rect{}), text_(std::move(text)) {}

    void paint(const Painter& p) override {
        const Theme& theme = p.theme();
        p.roundedRect(p.bounds(), p.bounds().h * 0.5f, accentColor(theme));
        p.text(text_ + "  (" + std::to_string(clicks_) + ")",
               Color{255, 255, 255, 255});
    }

    bool onPress(float, float) override {
        ++clicks_;
        std::printf("badge clicked %d time(s)\n", clicks_);
        return true;
    }

    float preferredWidth(const Theme& theme, const Window&) const override {
        return 40.0f + float(text_.size()) * theme.metrics.text_size * 0.62f;
    }

    float preferredHeight(const Theme&, const Window&) const override {
        return 34.0f;
    }

private:
    std::string text_;
    int         clicks_ = 0;
};

int main() {
    Window win = Window::open("SimpleWidget badge", 520, 260);
    if (!win) return 1;

    auto& col = win.verticalLayout();
    col.setPadding(28.0f);
    col.setSpacing(14.0f);

    col.add<Label>("A custom widget with two overrides:")
        .setTextSize(15.0f);
    col.add<Badge>("Click me");
    col.add<Label>("The layout uses the badge's preferred height.")
        .setTextSize(12.0f);

    win.run();
}
```

Things to notice:

- The badge never sets its bounds. `AutoSize::Both` is the default, so the
  vertical layout uses `preferredHeight` for the row; the row keeps the column
  width. A horizontal layout would use `preferredWidth` instead.
- `paint` receives a `Painter`, not an `NVGcontext`. `Painter` draws in the
  widget's coordinate space and uses the theme text size by default.
- `onPress` returns `true` to tell the base class that the press was consumed.
  A return of `false` leaves the event for the widgets below.
- `accentColor(theme)` follows the theme and any per-widget override.

## Example 2 — a full Widget with animation and focus

The meter fills a rounded track, animates toward a target level, brightens on
hover, and can be driven with the mouse or the keyboard. It uses the full base
because it needs raw NanoVG, `tick`, focus and a key handler.

```cpp
#include <cstdio>

#include <GLFW/glfw3.h>
#include <nanovg.h>

#include <fluffy/animation.hpp>
#include <fluffy/fluffy.hpp>
#include <fluffy/inline.hpp>

using namespace fluffy;

// ––––––––[ LevelMeter ] animated bar with hover and keyboard control
class LevelMeter final : public Widget {
public:
    explicit LevelMeter(Rect bounds = Rect{}) : Widget(bounds) {
        levelAnim_.snapTo(level_);
    }

    void draw(NVGcontext* vg, const Theme& theme, const Window&) override {
        if (!visible_) return;

        const float radius = bounds_.h * 0.5f;
        const float inset  = 3.0f;
        const float value  = levelAnim_.value();

        // Track
        nvgBeginPath(vg);
        nvgRoundedRect(vg, bounds_.x, bounds_.y, bounds_.w, bounds_.h, radius);
        nvgFillColor(vg, nvgRGBA(theme.colors.border.r, theme.colors.border.g,
                                 theme.colors.border.b, 70));
        nvgFill(vg);

        // Fill: the colour mixes from accent to pressed accent on hover.
        const float fill_w = (bounds_.w - inset * 2.0f) * value;
        if (fill_w > 0.5f) {
            Color fill = mix(accentColor(theme), accentPressedColor(theme),
                             hoverAnim_.value());
            nvgBeginPath(vg);
            nvgRoundedRect(vg, bounds_.x + inset, bounds_.y + inset,
                           fill_w, bounds_.h - inset * 2.0f, radius);
            nvgFillColor(vg, nvgRGBA(fill.r, fill.g, fill.b, fill.a));
            nvgFill(vg);
        }

        // Label
        char label[16];
        std::snprintf(label, sizeof(label), "%.0f%%", value * 100.0f);
        const bool on_fill = value > 0.45f;
        const Color ink = on_fill ? Color{255, 255, 255, 255}
                                  : theme.colors.on_surface;
        nvgFontFace(vg, "default-bold");
        nvgFontSize(vg, theme.metrics.text_size);
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, nvgRGBA(ink.r, ink.g, ink.b, ink.a));
        nvgText(vg, bounds_.x + bounds_.w * 0.5f,
                bounds_.y + bounds_.h * 0.5f, label, nullptr);

        if (focused_) drawFocusRing(vg, bounds_, theme);
    }

    EventResult handleEvent(const Event& event, const Window&) override {
        if (!visible_ || !enabled_) return EventResult::Ignored;

        if (const auto* down = std::get_if<MouseDownEvent>(&event)) {
            if (down->button == MouseButton::Left &&
                containsPoint(down->x, down->y)) {
                if (window_) window_->requestFocus(this);
                pressed_ = true;
                return EventResult::Consumed;
            }
        }
        if (const auto* up = std::get_if<MouseUpEvent>(&event)) {
            if (pressed_ && up->button == MouseButton::Left) {
                pressed_ = false;
                if (containsPoint(up->x, up->y)) cycle();
                return EventResult::Consumed;
            }
        }
        if (const auto* key = std::get_if<KeyDownEvent>(&event)) {
            if (focused_ &&
                (key->key == GLFW_KEY_SPACE || key->key == GLFW_KEY_ENTER)) {
                cycle();
                return EventResult::Consumed;
            }
        }
        return EventResult::Ignored;
    }

    void updateHover(float mx, float my) noexcept override {
        const bool hovered = visible_ && enabled_ && containsPoint(mx, my);
        hoverAnim_.setTarget(hovered ? 1.0f : 0.0f);
        if (hovered && window_) window_->setCursorShape(CursorShape::Hand);
    }

    void tick(float dt, const Theme& theme) noexcept override {
        levelAnim_.tick(dt, 0.18f);
        hoverAnim_.tick(dt, theme.metrics.animation.hover_tau);
    }

    float naturalWidth(const Theme&, const Window&) const override {
        return 260.0f;
    }

    float naturalHeight(const Theme&, const Window&) const override {
        return 36.0f;
    }

    bool isFocusable() const noexcept override { return true; }

    void setFocused(bool focused) noexcept override { focused_ = focused; }

private:
    void cycle() {
        level_ = level_ >= 0.95f ? 0.1f : level_ + 0.25f;
        levelAnim_.setTarget(level_);
        std::printf("level %.0f%%\n", level_ * 100.0f);
    }

    float      level_     = 0.35f;
    SmoothAnim levelAnim_;
    SmoothAnim hoverAnim_;
    bool       pressed_   = false;
    bool       focused_   = false;
};

int main() {
    Window win = Window::open("Custom Widget", 560, 260);
    if (!win) return 1;

    auto& col = win.verticalLayout();
    col.setPadding(28.0f);
    col.setSpacing(14.0f);

    col.add<Label>("A Widget subclass with animation and focus:")
        .setTextSize(15.0f);
    col.add<LevelMeter>(Rect{});
    col.add<Label>("Click the bar, or focus it and press Space.")
        .setTextSize(12.0f);

    win.run();
}
```

What this example adds over the badge:

- **Raw NanoVG.** `draw` emits paths and text itself, using `nvgFontFace` and
  `nvgTextAlign` and reading colors from the theme.
- **Two animation channels.** `levelAnim_` follows the level target; `hoverAnim_`
  follows the pointer. Both advance in `tick` with `dt`, and `draw` only reads
  `value()`. Changing the level or the hover state sets a target; it never jumps.
- **Press and release.** The press is remembered and consumed; the action fires
  on release inside the widget.
- **Focus.** A click requests focus, `setFocused` records it, the Space/Enter key
  is handled only while focused, and `drawFocusRing` shows the standard outline.
- **Cursor.** `updateHover` sets the hand cursor while the pointer is over the
  bar and the window resets the arrow for everything else.
- **Sizing.** `naturalWidth`/`naturalHeight` supply the layout size.

## Using a custom widget in YAML

The YAML loader creates widgets by type name through `WidgetFactory`. Register
your class before loading the file, and implement `setProperty` to read custom
keys (`text`, `level`, `url`…). Always call the base version for the keys you do
not handle, so `x`, `width`, `visible`, `font`, `accent`, constraints and the
other common properties keep working:

```cpp
void setProperty(std::string_view name, const YamlNode& value) override {
    if (name == "level") {
        level_ = std::clamp(value.asFloat(), 0.0f, 1.0f);
        levelAnim_.snapTo(level_);
        return;
    }
    Widget::setProperty(name, value);
}

WidgetFactory::registerWidget("LevelMeter",
    []() -> std::unique_ptr<Widget> { return std::make_unique<LevelMeter>(); });
```

Register once at startup, before `load`/`loadFromString`. `setEvent(name, handler)`
and `triggerEvent(name, args)` connect YAML event names to handlers if you use
them.

## Checklist and common pitfalls

- Return `Consumed` from `handleEvent` only when you actually used the event.
- Check `visible_`/`enabled_` in `draw` and `handleEvent`; the window checks them
  for hover and tick.
- Set the font, fill and stroke you need in every `draw`; NanoVG state is sticky.
- Wrap transform or scissor changes in `nvgSave`/`nvgRestore`.
- Use `bounds()` coordinates directly; do not apply a device-pixel scale.
- Advance animations in `tick(dt, theme)` and read them in `draw`; never call
  `glfwGetTime` or sleep in `draw`.
- Let layouts size layout children. Override `naturalWidth`/`naturalHeight` (or
  `preferredWidth`/`preferredHeight` for `SimpleWidget`) instead of hard-coding
  bounds.
- Use `accentColor(theme)` and the theme colors so custom widgets follow light,
  dark and custom themes.
- Request focus from `window_` on click, and handle keys only when `focused_`.
- Register YAML types with `WidgetFactory` before loading a UI file.

## Where to go next

- [Widget basics](widget.md) — the base class, sizing and themes.
- [Geometry & colors](geom.md) — `Rect`, `Color` and `mix`.
- [Tutorial 11](tutorial/11-custom-widget.md) — a complete `Link` widget with
  `Inline` content.
- [YAML UI](yaml.md) — how declarative files instantiate widgets.
