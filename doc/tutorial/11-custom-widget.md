# Tutorial 11 — Custom Widget (Link)

Build your own widget: a clickable hyperlink with a hover underline and `Inline`
content.

## What you will build

A `Link` widget that draws text or `Inline` content, reacts to clicks, changes the
mouse cursor on hover, animates an underline, and can be registered for YAML UI
files. The window places a plain link and an icon link.

## What you will learn

- How a widget implements `draw` and `handleEvent`.
- How to react to hover and animate it with `SmoothAnim`.
- How to give a widget size hints for layouts.
- How `setProperty` and `WidgetFactory` make a widget usable from YAML.
- When the smaller `SimpleWidget` base is enough.

## The words you need

- **Custom widget** — a class that derives from `Widget`. The window owns it and
  treats it like any built-in widget.
- **`draw`** — called every frame. It receives a NanoVG context, the current
  `Theme`, and the `Window`, and paints the widget's appearance.
- **`handleEvent`** — called for input events such as mouse presses and releases.
  Return `EventResult::Consumed` when the widget used the event, and
  `EventResult::Ignored` otherwise.
- **Hover** — being under the mouse. `updateHover` receives the mouse position;
  use it to update hover state and the cursor shape.
- **Animation** — `SmoothAnim` eases a value toward a target. `setTarget` chooses
  the goal, `tick(dt, tau)` advances the value each frame, and `value()` reads it.
- **`Inline`** — as in [Tutorial 5](05-icons-and-themes.md), `Inline` holds a mix
  of icon and text runs in one piece of content.
- **Widget factory** — a registry that maps a YAML type name to a function that
  creates the widget. [Tutorial 6](06-yaml-ui.md) introduced YAML UI files.
- **Application** — as in [Tutorial 10](10-multiwindow.md), `Application::get()`
  returns the shared event loop. `run()` blocks until every window is closed.
- **Dialog** — as in [Tutorial 7](07-dialogs.md), `win.showDialog(...)` opens a
  small modal window and waits for the user to answer.

> This `Link` derives from `Widget` because it needs hover animation and a focus
> ring. If you only need to paint something and react to a press, derive from
> `SimpleWidget` and override `paint` and `onPress` instead.

## The full program

Create `main.cpp`. The `Link` class lives in the same file for brevity; in a real
project, move it to `link.hpp` and `link.cpp`.

```cpp
#include <cstdio>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <nanovg.h>

#include <fluffy/animation.hpp>
#include <fluffy/application.hpp>
#include <fluffy/factory.hpp>
#include <fluffy/icons.hpp>
#include <fluffy/inline.hpp>
#include <fluffy/label.hpp>
#include <fluffy/theme.hpp>
#include <fluffy/widget.hpp>
#include <fluffy/window.hpp>
#include <fluffy/yaml_fwd.hpp>

using namespace fluffy;

// ––––––––[ Link ] clickable text with hover underline
class Link final : public Widget {
public:
    using ClickHandler = std::function<void()>;

    Link() = default;

    explicit Link(std::string text, std::string url = "")
        : Widget(Rect{}), text_(std::move(text)), url_(std::move(url)) {}

    Link(Rect bounds, std::string text, std::string url = "")
        : Widget(bounds), text_(std::move(text)), url_(std::move(url)) {}

    Link(Rect bounds, Inline content)
        : Widget(bounds), inline_(content.runs()), is_inline_(true) {}

    void setOnClick(ClickHandler handler) { on_click_ = std::move(handler); }

    Link& onClick(ClickHandler handler) {
        on_click_ = std::move(handler);
        return *this;
    }

    const std::string& text() const noexcept { return text_; }

    void setText(std::string text) {
        text_ = std::move(text);
        is_inline_ = false;
    }

    void setUrl(std::string url) { url_ = std::move(url); }

    void draw(NVGcontext* vg, const Theme& theme, const Window&) override {
        if (!visible_) return;

        float size = textSize_ > 0 ? textSize_ : theme.metrics.text_size;
        Color base = hovered_ ? accentColor(theme) : theme.colors.accent;
        float t = hoverAnim_.value();
        Color color = mix(base, Color{base.r, base.g, base.b, 200}, t * 0.2f);

        float x = bounds_.x;
        float y = bounds_.y + bounds_.h * 0.5f;

        if (is_inline_) {
            renderInline(vg, inline_, x, y, NVG_ALIGN_LEFT, NVG_ALIGN_MIDDLE,
                         size, 1.2f, 0.1f, "default", color, color);
        } else {
            nvgFontFace(vg, "default");
            nvgFontSize(vg, size);
            nvgFillColor(vg, nvgRGBA(color.r, color.g, color.b, color.a));
            nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
            nvgText(vg, x, y, text_.c_str(), nullptr);
        }

        if (hovered_ || pressed_) {
            float width = 0.0f;
            if (is_inline_) {
                for (const auto& run : inline_) {
                    nvgFontFace(vg, "default");
                    nvgFontSize(vg, run.size > 0 ? run.size : size);
                    width += run.type == Inline::Run::Type::SvgIcon
                        ? size * 1.2f
                        : nvgTextBounds(vg, 0, 0, run.utf8.c_str(), nullptr, nullptr);
                }
            } else {
                width = nvgTextBounds(vg, 0, 0, text_.c_str(), nullptr, nullptr);
            }

            nvgBeginPath(vg);
            nvgMoveTo(vg, x, y + size * 0.4f);
            nvgLineTo(vg, x + width, y + size * 0.4f);
            nvgStrokeColor(vg, nvgRGBA(color.r, color.g, color.b, 180));
            nvgStrokeWidth(vg, 1.0f);
            nvgStroke(vg);
        }

        if (focused_) drawFocusRing(vg, bounds_, theme);
    }

    EventResult handleEvent(const Event& event, const Window&) override {
        if (!visible_ || !enabled_) return EventResult::Ignored;

        if (auto* down = std::get_if<MouseDownEvent>(&event)) {
            if (down->button == MouseButton::Left &&
                containsPoint(down->x, down->y)) {
                pressed_ = true;
                return EventResult::Consumed;
            }
        }

        if (auto* up = std::get_if<MouseUpEvent>(&event)) {
            if (pressed_ && up->button == MouseButton::Left) {
                pressed_ = false;
                if (containsPoint(up->x, up->y)) {
                    if (on_click_) on_click_();
                    else triggerEvent("onClick");
                    return EventResult::Consumed;
                }
            }
            pressed_ = false;
        }

        return EventResult::Ignored;
    }

    void updateHover(float mx, float my) noexcept override {
        bool was_hovered = hovered_;
        hovered_ = visible_ && enabled_ && containsPoint(mx, my);
        if (hovered_ != was_hovered) {
            hoverAnim_.setTarget(hovered_ ? 1.0f : 0.0f);
        }
        if (hovered_ && window_) window_->setCursorShape(CursorShape::Hand);
    }

    void tick(float dt, const Theme& theme) noexcept override {
        hoverAnim_.tick(dt, theme.metrics.animation.hover_tau);
    }

    float naturalWidth(const Theme& theme, const Window&) const override {
        float size = textSize_ > 0 ? textSize_ : theme.metrics.text_size;
        if (is_inline_) return 120.0f;
        return float(text_.size()) * size * 0.6f + 8.0f;
    }

    float naturalHeight(const Theme&, const Window&) const override {
        return 28.0f;
    }

    void setProperty(std::string_view name, const YamlNode& value) override {
        if (name == "text") {
            setText(value.asString());
            return;
        }
        if (name == "url") {
            setUrl(value.asString());
            return;
        }
        Widget::setProperty(name, value);
    }

    bool isFocusable() const noexcept override { return true; }

    void setFocused(bool focused) noexcept override { focused_ = focused; }

private:
    std::string text_;
    std::string url_;
    std::vector<Inline::Run> inline_;
    bool is_inline_ = false;
    bool hovered_ = false;
    bool pressed_ = false;
    bool focused_ = false;
    SmoothAnim hoverAnim_;
    ClickHandler on_click_;
};

int main() {
    Window win = Window::open("Link Demo", 640, 320);
    if (!win) return 1;

    auto& col = win.verticalLayout();
    col.setPadding(32.0f);
    col.setSpacing(12.0f);

    auto& plain = col.add<Link>(
        Rect{}, "https://fluffy.example.com", "https://fluffy.example.com");
    plain.onClick([] {
        std::printf("clicked link\n");
    });

    Inline withIcon = Inline{} << Icons::link << " Visit docs";
    auto& iconLink = col.add<Link>(Rect{}, withIcon);
    iconLink.onClick([&win] {
        win.showDialog("Link", "Inline link clicked");
    });

    WidgetFactory::registerWidget("Link",
        []() -> std::unique_ptr<Widget> { return std::make_unique<Link>(); });

    auto& hint = col.add<Label>("Click a link above");
    hint.setTextSize(12.0f);

    Application::get().run();
}
```

## Build and run

Compile this program the same way you compiled the first tutorial, then run the
resulting executable. If you need the build commands, revisit
[Tutorial 1](01-hello-fluffy.md#build-and-run).

## Step by step

### 1. Choose `SimpleWidget` or the full `Widget`

Every widget derives from `Widget` and implements two required methods: `draw`
and `handleEvent`. `Widget` also offers optional hooks such as `updateHover`,
`tick`, `naturalWidth`, `naturalHeight`, and `setProperty`. The `Link` uses those
hooks because it animates and can be described in YAML.

For a widget that only paints itself and reacts to a press, `SimpleWidget` keeps
the code smaller: override `paint(const Painter&)` and `onPress(x, y)` instead of
working with `NVGcontext*` and raw events.

### 2. Construct the links and place them in a window

```cpp
Window win = Window::open("Link Demo", 640, 320);
if (!win) return 1;

auto& col = win.verticalLayout();
col.setPadding(32.0f);
col.setSpacing(12.0f);

auto& plain = col.add<Link>(
    Rect{}, "https://fluffy.example.com", "https://fluffy.example.com");
plain.onClick([] {
    std::printf("clicked link\n");
});

Inline withIcon = Inline{} << Icons::link << " Visit docs";
auto& iconLink = col.add<Link>(Rect{}, withIcon);
iconLink.onClick([&win] {
    win.showDialog("Link", "Inline link clicked");
});
```

The vertical layout from [Tutorial 4](04-layouts.md) stacks the widgets and assigns
their geometry, so the links are created with an empty `Rect`. `add<Link>`
returns a reference, which lets you attach a click callback immediately. The first
link is plain text with a URL; the second is built from `Inline` content and
combines `Icons::link` with the label `" Visit docs"`.

### 3. Draw with `Theme`

`draw` receives the NanoVG context, the current `Theme`, and the `Window`. The
`Link` uses `nvgFontFace`, `nvgFontSize`, `nvgFillColor`, and `nvgText` for plain
text, and `renderInline` for content that mixes icons and text. `accentColor(theme)`
returns the widget's own accent if you set one, or the theme's accent otherwise.

The hover color is blended with `mix`, using the animation value:

```cpp
Color base = hovered_ ? accentColor(theme) : theme.colors.accent;
float t = hoverAnim_.value();
Color color = mix(base, Color{base.r, base.g, base.b, 200}, t * 0.2f);
```

The underline is drawn only while the link is hovered or pressed. For plain text,
its width comes from `nvgTextBounds`; for `Inline` content, the code sums the icon
and text run widths. A focus ring is drawn when the widget has focus.

### 4. Handle mouse events

`handleEvent` receives one event at a time. For a click, the press must start and
end inside the widget:

```cpp
if (auto* down = std::get_if<MouseDownEvent>(&event)) {
    if (down->button == MouseButton::Left &&
        containsPoint(down->x, down->y)) {
        pressed_ = true;
        return EventResult::Consumed;
    }
}
```

On release, the link checks that the mouse is still inside before calling the
callback. Returning `EventResult::Consumed` tells Fluffy that the widget used the
event, so it does not need to be handled elsewhere.

### 5. Animate hover and provide size hints

```cpp
void updateHover(float mx, float my) noexcept override {
    bool was_hovered = hovered_;
    hovered_ = visible_ && enabled_ && containsPoint(mx, my);
    if (hovered_ != was_hovered) {
        hoverAnim_.setTarget(hovered_ ? 1.0f : 0.0f);
    }
    if (hovered_ && window_) window_->setCursorShape(CursorShape::Hand);
}

void tick(float dt, const Theme& theme) noexcept override {
    hoverAnim_.tick(dt, theme.metrics.animation.hover_tau);
}
```

When hover changes, `setTarget` moves the animation toward `0` or `1`, and `tick`
advances it using the theme's hover speed. `naturalWidth` and `naturalHeight` give
the layout size hints. The width of `Inline` content is approximate here; for a
precise layout, measure the runs or provide explicit bounds.

### 6. Make it YAML-ready

Override `setProperty` for the properties your widget adds, and forward everything
else to `Widget::setProperty` so common properties such as position, size,
visibility, and text size still work:

```cpp
void setProperty(std::string_view name, const YamlNode& value) override {
    if (name == "text") {
        setText(value.asString());
        return;
    }
    if (name == "url") {
        setUrl(value.asString());
        return;
    }
    Widget::setProperty(name, value);
}
```

Then register the type with `WidgetFactory` before loading a YAML file:

```cpp
WidgetFactory::registerWidget("Link",
    []() -> std::unique_ptr<Widget> { return std::make_unique<Link>(); });
```

With that registration in place, a YAML file can create a `Link` by type name and
set its `text` and `url` properties.

## Exercises

1. Give the link a visited color after the first click.
2. Add a `setInline` method and make `naturalWidth` measure the actual runs.
3. Bind `onClick` in YAML and handle the event with
   `win.dispatcher().registerHandler("open", ...)`.

## Next

See [Widget](../widget.md) for the full contract and [Button](../button.md) for a
built-in widget that also animates hover and press.
