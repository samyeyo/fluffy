# Widget

A `Widget` is anything you can put in a window — a label, a button, a text input, an image, or a container — and all widgets share a common set of properties and behaviors.

## What it's for

Use the common `Widget` API whenever you need to work with a widget without caring which concrete type it is. You never create `Widget` directly. You create concrete widgets like `Label`, `Button`, or `TextInput`, and they all inherit from `Widget`, so everything on this page applies to all of them.

A window **owns** the widgets you add to it. A widget lives for as long as the window, and you never delete it yourself.

## A first example

```cpp
auto& btn = win.add<fluffy::Button>("Click");
btn.setBounds(fluffy::Rect{40, 40, 120, 40});

btn.onClick([] {
    std::printf("Clicked!\n");
});
```

`win.add<fluffy::Button>(...)` creates the button inside the window and returns a reference to it. Because it is a reference, you call its methods with `.` — never `->`. Its bounds start empty (`Rect{}`), so you call `setBounds` when you position it by hand. `onClick` then hands the button a **callback** — a small function it calls when something happens, here written as a lambda — so it can run your code on a click.

In the examples below, `win` is an open window (see [Window](window.md)).

## Common tasks

### Chain configuration calls

`win.add<T>(...)`, `win.verticalLayout()`, and many setters return a reference to the object they just changed. That lets you configure a widget in one expression:

```cpp
auto& col = win.verticalLayout()
    .setPadding(24.0f)
    .setSpacing(8.0f)
    .align(fluffy::Align::Center)
    .justify(fluffy::Justify::Start);

col.add<fluffy::Label>("Welcome")
    .setTextSize(24.0f)
    .setHorizontal(fluffy::Label::Align::Center);

col.add<fluffy::Button>("Close")
    .onClick([&win] { win.requestClose(); });
```

Read the chain from left to right: each call returns a reference, and the next call runs on that same object. The pattern is optional — storing a reference is often clearer when you need the widget later. Only calls that return a reference can be chained. Plain setters such as `setBounds`, `setLabel`, `setInline`, and `setOnClick` return `void`. Setters inherited from `Widget`, such as `setFont`, return `Widget&`; call concrete-type methods first or split the chain when you need `Label&` or `Button&` again.

### Position, size, and weight a widget

```cpp
auto& btn = win.add<fluffy::Button>("Click"); // bounds default to Rect{}
btn.setBounds(fluffy::Rect{40, 40, 120, 40}); // only when positioning by hand

btn.bounds();              // current rectangle
btn.setBounds(newRect);    // move or resize
btn.width();              // 120
btn.height();             // 40
btn.containsPoint(x, y);  // is the point inside?
```

A widget's **bounds** are the `Rect` that says where it sits and how big it is. See [Geometry & colors](geom.md) for `Rect` itself.

When a layout distributes extra space, you can give a widget a larger share:

```cpp
btn.setStretchWeight(2.0f);       // share of extra space in layouts
fluffy::Window* w = btn.window();
```

`window()` gives you the window that currently owns the widget.

### Let a widget size itself

Normally a widget uses the `Rect` you gave it. You can ask it to size itself from its content instead:

```cpp
btn.setAutoSize(fluffy::AutoSize::Width);  // width from text
btn.setAutoSize(fluffy::AutoSize::Height); // height from text
btn.setAutoSize(fluffy::AutoSize::Both);   // both from content
btn.setAutoSize(fluffy::AutoSize::Disabled); // fixed size
```

`AutoSize::Both` is the **default** for all widgets. The layout respects this: a widget with `AutoSize::Both` is sized by its `naturalWidth()` / `naturalHeight()`, while `AutoSize::Disabled` lets the layout assign the slot size. `AutoSize::Width` or `AutoSize::Height` locks one axis and lets the content drive the other.

`Label` and `Button` compute their natural size from their text. `Container` computes its natural size from its children and layout. You can inspect the current setting and the resulting natural size:

```cpp
auto as = btn.autoSize();
float w = btn.naturalWidth(win.theme(), win);
float h = btn.naturalHeight(win.theme(), win);
```

### Control visibility, enabled state, and focus

```cpp
btn.setVisible(false);  // hidden and ignored by mouse
btn.setEnabled(false);  // visible but does not react
```

Whether a widget can receive keyboard focus depends on the widget. `Widget::isFocusable()` is `false` at the base level, and concrete widgets such as `Button` and `TextInput` override it. `setFocused()` is a no-op on the base class:

```cpp
btn.setFocused(true);
btn.clearFocus();              // unfocus (calls setFocused(false))
```

`isFocused()` exists only on concrete widgets (for example `Button::isFocused()`), not on the `Widget` base. Use `Window::requestFocus(w)` and `Window::clearFocus()` to move focus between widgets.

### Set fonts and read text state

```cpp
btn.setFont("Inter");          // by family name (loaded at runtime)
btn.setFont("mem:body");       // from pre-registered memory
btn.setFont("file:/path.ttf"); // from disk (absolute or relative)
btn.setFont("/path.ttf");      // auto-detected by .ttf/.ttc/.otf extension
btn.setTextSize(18.0f);        // per-widget size override
```

Set the font to `"default"` or the size to `0.0f` to revert to the theme. You can read the current values back:

```cpp
std::string f = btn.font();       // "" means "use theme font"
float sz = btn.textSize();        // 0.0f means "use theme size"
```

### Customize, tag, introspect, and drive events

A widget can override the theme accent with its own colors:

```cpp
btn.setAccentOverride(fluffy::Color::fromHex(0xFF4F8DFF));
btn.setAccentPressedOverride(fluffy::Color::fromHex(0xFF2C66D8));
btn.clearAccentOverride();        // back to theme accent
```

You can ask what a widget is and walk its children:

```cpp
btn.isFocusable();   // can it take focus?
btn.fillsParent();   // root container only
btn.isContainer();   // Container
btn.isMenu();        // Menu
btn.isTooltip();     // Tooltip
btn.forEachChild([](fluffy::Widget* c) { /* ... */ });
```

An arbitrary string can be attached to a widget. This is useful for finding widgets loaded from YAML:

```cpp
btn.userdata() = "save-button";
```

Widgets can also be configured through the YAML path and can fire named events:

```cpp
btn.setProperty("width", node);   // YAML path (also x/y/height/visible/enabled/font/...)
btn.setEvent("onClick", "save");  // YAML path: bind a dispatcher handler

btn.triggerEvent("customEvent", {});
```

Events route through the window's dispatcher. Most of the time you use the widget's typed helpers instead, like `onClick`, `onChange`, `onSelect`, or `onSubmit` (chainable; `setOnClick` and the other `set*` methods are the plain setters).

## Good to know

- `AutoSize::Both` is the default for all widgets.
- Many setters return a reference to the same object and can be chained; see [Chain configuration calls](#chain-configuration-calls).
- `Widget::isFocusable()` is `false` at the base level. `setFocused()` is a no-op on the base class, and `isFocused()` exists only on concrete widgets.
- Setting a widget's font to `"default"` or its text size to `0.0f` reverts that property to the theme.
- Widgets have per-frame hooks that the window calls automatically: `draw(...)` renders the widget, `handleEvent(...)` reacts to input, `updateHover(...)` tracks the mouse position, `tick(...)` updates animations, and `onResize(...)` reacts to window resize. You rarely call these yourself unless you write a custom widget.
- If you write a custom widget, start from `SimpleWidget`: override `paint(const Painter&)` and `onPress(x, y)` instead of raw `draw(NVGcontext*, ...)` plus `handleEvent(Event, ...)`. See Tutorial 11.

## Quick reference

| Area | API |
|---|---|
| Geometry | `bounds()`, `setBounds(Rect)`, `width()`, `height()`, `containsPoint(x, y)` |
| Layout | `setStretchWeight(float)`, `window()` |
| Auto sizing | `setAutoSize(AutoSize::Width)`, `setAutoSize(AutoSize::Height)`, `setAutoSize(AutoSize::Both)`, `setAutoSize(AutoSize::Disabled)`, `autoSize()` |
| Natural size | `naturalWidth(theme, win)`, `naturalHeight(theme, win)` |
| Visibility | `setVisible(bool)` |
| Enabled state | `setEnabled(bool)` |
| Focus | `setFocused(bool)`, `clearFocus()`, `isFocusable()`, `Window::requestFocus(w)`, `Window::clearFocus()` |
| Typography | `setFont(spec)`, `setTextSize(float)`, `font()`, `textSize()` |
| Accent | `setAccentOverride(Color)`, `setAccentPressedOverride(Color)`, `clearAccentOverride()` |
| Introspection | `isFocusable()`, `fillsParent()`, `isContainer()`, `isMenu()`, `isTooltip()`, `forEachChild(...)` |
| Tagging | `userdata()` |
| YAML | `setProperty(name, node)`, `setEvent(name, handler)` |
| Events | `triggerEvent(name, args)` |
| Frame hooks | `draw(...)`, `handleEvent(...)`, `updateHover(...)`, `tick(...)`, `onResize(...)` |

## See also

- [Label](label.md) · [Button](button.md) · [TextInput](textinput.md) · [Container](container.md)
- [Custom widgets](custom.md) — write your own `SimpleWidget` or `Widget`.
- [Window](window.md) — where widgets live.
