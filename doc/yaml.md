# YAML UI loader

YAML is a plain-text format that uses indentation to describe nested data. Fluffy can read a YAML file that describes a tree of widgets and build those widgets in a window for you. That lets you keep the shape of a screen in a file instead of in C++ code.

## What it's for

Use the YAML UI loader when you want to separate layout from code. You describe widgets and containers in YAML, then load them into a window. This is useful for keeping the structure of a screen out of your C++ source, and for changing a screen without recompiling.

## The words you need

- **YAML** — a text format that uses indentation to describe nested data. Here it describes a tree of widgets instead of C++ calls.
- **Window** — the operating-system window your app draws into. `Window::open` creates one.
- **Entry** — one named block in the YAML tree. `Label:` is an entry, and the lines indented under it are that label's properties.
- **Container** — a widget that holds other widgets. It is how you group a screen into one loadable piece.
- **Layout entry** — a map entry inside a `Container` that names a layout (`Horizontal`, `Vertical` or `Grid`) and tells the container how to arrange the entries that follow. Put it first, before the widget entries.
- **Callback** — a small function you register under a name. In YAML you write the name, not the code.
- **Dispatcher** — the per-window registry that matches a name in the YAML to the callback registered under that name.
- **Loading** — `win.load("ui.yaml")` reads the file and adds its widgets to the window. It reports whether the file could be loaded so you can stop with a clear message.

## A first example

Create a file named `ui.yaml`:

```yaml
Label:
  text: "Hello, World!"
  x: 40
  y: 40
  width: 200
  height: 32
```

Then load it from a program:

```cpp
#include <cstdio>

#include <fluffy/fluffy.hpp>

int main() {
    fluffy::Window win = fluffy::Window::open("YAML UI", 800, 600);
    if (!win) return 1;

    auto loaded = win.load("ui.yaml");
    if (!loaded) {
        std::printf("Failed to load UI: %s\n", loaded.error().c_str());
    }

    win.run();
}
```

`Window::open` creates the window, and `if (!win)` checks that it opened successfully. The YAML creates a single `Label` at `(40, 40)` with size `200×32`. Each widget is a map with one type key (`Label:`), and the lines indented under it are that widget's properties. Use two spaces per indentation level.

`win.load("ui.yaml")` reads the file and adds its widgets to the window. It reports whether the file could be loaded: when `loaded` is true, the content was built; when it is false, `loaded.error()` returns the message to print. Put `ui.yaml` in the directory you run the program from.

The snippets on the rest of this page assume `win` is the window opened above.

## Common tasks

### Load a UI from a string

Use `loadFromString` when the YAML is already in memory instead of in a file:

```cpp
auto loaded = win.loadFromString(
    "Label:\n"
    "  text: \"Hello\"\n"
    "  x: 40\n"
    "  y: 40\n"
    "  width: 200\n"
    "  height: 32\n");
if (!loaded) {
    std::printf("Failed to load UI: %s\n", loaded.error().c_str());
}
```

The string has the same structure as the file, with newlines written as `\n`.

### Build a container with a layout

A `Container` holds other widgets. When you want a layout, make it the first map child, before the widget entries:

```yaml
Container:
  x: 0
  y: 0
  width: 800
  height: 600
  Vertical:
    spacing: 16
    padding: 24
  Label:
    text: "Welcome"
  Button:
    text: "Start"
    onClick: "start"
```

The `Vertical` layout places children top-to-bottom with 16 pixels between them and 24 pixels of padding inside the container. `Horizontal` places them left-to-right, and `Grid` arranges them in columns. The `Label` and `Button` entries after the layout become the container's children.

`onClick: "start"` stores a callback name. The [Connect callbacks](#connect-callbacks) section shows how to register the code for that name.

### Configure widgets

**Label**

```yaml
Label:
  text: "Hello"
  x: 40
  y: 40
  width: 200
  height: 32
  align: center
```

Alignment keys are `align` (both axes), `horizontal_align` and `vertical_align`. Note: a plain `color` key is parsed but currently has no effect; use an `inline` color run instead.

**Button**

```yaml
Button:
  text: "Save"
  x: 40
  y: 120
  width: 120
  height: 40
  onClick: "save"
```

There is no `dropdownMenu` key — attach menus in code.

**Rich text with `inline`**

`Label` and `Button` accept either a plain `text` key or an `inline` run list. Use `inline` when you want icons, colors, or mixed styles in one label. The two keys are mutually exclusive: if both are present, loading fails.

```yaml
Button:
  inline:
    - icon: check
    - text: " Save"
    - color: "#FF4F8DFF"
    - text: " (draft)"
    - reset: true
    - text: "!"
  onClick: "save"
```

Each list item is one run. `text` adds text, `icon` adds a built-in Lucide icon by name (for example `check`, `heart`, or `rocket`), and `color`, `size`, `bold`, and `italic` change the style of the runs that follow. `reset` clears the current style. Style runs produce no visible content by themselves. `onClick` stays a normal sibling key; it is not part of the inline content.

**TextInput**

```yaml
TextInput:
  text: "Hello"
  placeholder: "Search…"
  multiline: false
  growable: true
  x: 40
  y: 80
  width: 300
  height: 36
  onSubmit: "search"
```

**WebView**

```yaml
WebView:
  url: "https://example.com"
  x: 20
  y: 20
  width: 680
  height: 320
  radius: 12
```

Use `url` for a URL and `html` for raw markup. Add `base_url` when the HTML
references relative assets. Use one source per view; the last `url` or `html`
key wins. `radius` accepts a fixed corner radius, or `-1` to track the theme.
Message handlers still need to be attached in C++ with `setOnMessage`.

**More widgets**

The Quick reference below lists every supported widget type and its special keys. A few limits to keep in mind: there is no `Image` YAML support, so create images in code, and `Menu` items plus `Button` dropdown menus are also built in code.

**Base properties**

Every widget also accepts the base properties: `x`, `y`, `w`/`width`, `h`/`height`, `visible`, `enabled`, `font`, `font_size`/`text_size`, `userdata`, `accent`, and `autoSize`. `autoSize` accepts `disabled`, `none`, `width`, `w`, `height`, `h`, or `both`.

### Connect callbacks

A window keeps a dispatcher: a small registry that matches a name to a callback. In YAML an event key stores the name; at runtime Fluffy looks up that name and calls the matching callback.

Any `onXxx` scalar key (for example `onClick`, `onChange`, `onSubmit`) is the name of a callback registered on the window's dispatcher. Non-scalar values are silently dropped. In practice `Button` fires `"onClick"` and `TextInput` fires `"onSubmit"`:

```yaml
Button:
  text: "Click me"
  onClick: "sayHello"
```

```cpp
win.dispatcher().registerHandler("sayHello",
    [](const fluffy::EventArgument&) {
        std::printf("Hello!\n");
    });
```

The string value of `onClick` is the callback name. Register that name on the window's dispatcher, and the callback runs when the button is clicked. The lambda receives an `EventArgument` with details about the event; this example ignores it and prints a line.

## Complete example

```yaml
Container:
  x: 0
  y: 0
  width: 800
  height: 600
  Vertical:
    spacing: 16
    padding: 40
    align_items: middle
  Label:
    text: "Welcome"
    width: 200
    height: 32
    align: center
  TextInput:
    placeholder: "Your name"
    width: 300
    height: 36
  Button:
    text: "Greet"
    width: 120
    height: 40
    onClick: "greet"
```

```cpp
#include <cstdio>

#include <fluffy/fluffy.hpp>

int main() {
    fluffy::Window win = fluffy::Window::open("YAML UI", 800, 600);
    if (!win) return 1;

    win.dispatcher().registerHandler("greet",
        [](const fluffy::EventArgument&) {
            std::printf("Greetings!\n");
        });

    auto loaded = win.load("ui.yaml");
    if (!loaded) {
        std::printf("Failed to load UI: %s\n", loaded.error().c_str());
        return 1;
    }

    win.run();
}
```

This screen uses a `Vertical` layout with `spacing`, `padding`, and `align_items`, then places a label, a text input, and a button inside it. The button's `onClick` key names the `"greet"` callback, which is registered on the window's dispatcher before the file is loaded.

## Good to know

- A plain `color` key on a `Label` is parsed but currently has no effect; put `color` runs inside `inline` instead.
- `text` and `inline` are mutually exclusive on `Label` and `Button`.
- There is no `dropdownMenu` key for `Button` — attach menus in code.
- `Menu` has no item keys — build items in code.
- For `Table`, `rows:` is silently ignored.
- For `TabView`, other keys such as geometry are ignored.
- There is no `Image` YAML support — create images in code.
- Non-scalar callback-name values are silently dropped.
- Fluffy's loader accepts repeated widget-type keys, so two `TextInput:` entries in the same container both become fields. A generic YAML linter may warn about the repeated keys; Fluffy itself accepts the file.

## Quick reference

### Layout properties

`Horizontal`, `Vertical` and `Grid` support:

| Property | Meaning |
|---|---|
| `spacing` | Space between children |
| `padding` | Space inside the container |
| `margin` | Per-side inset on both axes |
| `align` / `align_items` | `start`/`left`/`top`, `center`/`centre`/`middle`, `end`/`right`/`bottom` (case-insensitive) |
| `justify` / `justify_content` | `start`, `center`, `end`, `space-between`, `space-evenly` |
| `cols` / `columns` | `Grid` column count |
| `rows` | `Grid` row count (`0` = auto) |

### Widget keys

`Button` (`text` or `inline`), `Checkbox` (`label`, `checked`), `ColorPicker`
(`color`), `ComboBox` (`placeholder`, `selected`|`value`, `items`|`options`
sequence), `Container` (`scrollable`), `Label` (`text` or `inline`), `ListView`
(`items` sequence, `selected` int), `Menu`
(no item keys — build items in code), `Nav` (`selected`), `ModalWindow`,
`ProgressRing` (`value`, `indeterminate`), `RadioButton` (`label`, `group`,
`checked`), `Slider` (`value`, `min`, `max`, `show_value`, `value_format`),
`Switch` (`label`, `checked`), `Table` (`columns` sequence or comma-separated
scalar — `rows:` is silently ignored), `TabView` (`tabs`, `selected`; other
keys such as geometry are ignored for `TabView`), `TextInput`, `Tooltip` (base
properties only), `WebView` (`url`, `html`, `base_url`, `radius`).

### Inline run keys

Inside `inline:` each item accepts one key: `text`, `icon`, `color`, `size`,
`bold`, `italic`, or `reset`. `color` and `size` apply to following runs;
`bold`/`italic` accept `true` or `false`; `reset: true` clears the style.

### Base properties

Every widget accepts: `x`, `y`, `w`/`width`, `h`/`height`, `visible`,
`enabled`, `font`, `autoSize` (`disabled`|`none`|`width`|`w`|`height`|`h`|
`both`), `font_size`|`text_size`, `userdata`, `accent`.

### Event callback keys

Any `onXxx` scalar key is the name of a callback registered on the window's
dispatcher. Non-scalar values are silently dropped.

## See also

- [Container](container.md) · [Layouts](layouts.md)
- [Window](window.md#show-dialogs-and-wire-events)
- `examples/yamlui` in the repository
