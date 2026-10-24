# Tutorial 6 — YAML UI

Describe your interface in a YAML file, load it at runtime, and connect its events to C++ handlers as you build a small login screen.

## What you will build

A login screen with a title, a username field, a password field, and a sign-in button. The layout and widget definitions live in `ui.yaml`; the C++ side registers the `login` handler and loads the file.

## What you will learn

- The structure of a Fluffy YAML file.
- How to load a YAML file into a window.
- How to bind YAML events to C++ handlers.

## The words you need

- **YAML** — a text format that uses indentation to describe nested data. Here it describes a tree of widgets instead of C++ calls.
- **Entry** — one named block in the YAML tree. `Label:` is an entry, and the lines indented under it are that label's properties.
- **Layout entry** — the first map entry inside a `Container`. It is the layout, not a widget, and it tells the container how to arrange the entries that follow.
- **Handler** — a callback registered under a name. YAML events store the name, not the code.
- **Dispatcher** — the per-window registry that matches a YAML event name to the handler registered under that name.
- **Loading** — `win.load("ui.yaml")` reads the file and adds its widgets to the window. It reports whether the file could be loaded so you can stop with a clear message.

The [YAML UI reference](../yaml.md) covers more widget types and keys.

## The YAML file

Create `ui.yaml` in the directory you will run the program from:

```yaml
Container:
  x: 0
  y: 0
  width: 640
  height: 400
  Vertical:
    spacing: 16
    padding: 40
  Label:
    text: "Login"
    text_size: 24
  TextInput:
    placeholder: "Username"
    onSubmit: "login"
  TextInput:
    placeholder: "Password"
  Button:
    text: "Sign in"
    onClick: "login"
```

> **About repeated keys:** YAML normally expects each key once in a block. Fluffy's loader reads entries in order and accepts repeated widget-type keys, so the two `TextInput:` entries both become fields. A generic YAML linter may warn about this; Fluffy itself accepts the file.

## The C++ code

```cpp
#include <cstdio>

#include <fluffy/fluffy.hpp>

int main() {
    fluffy::Window win = fluffy::Window::open("YAML UI", 640, 400);
    if (!win) return 1;

    win.dispatcher().registerHandler("login",
        [](const fluffy::EventArgument&) {
            std::printf("Signing in...\n");
        });

    auto loaded = win.load("ui.yaml");
    if (!loaded) {
        std::printf("Failed to load UI: %s\n", loaded.error().c_str());
        return 1;
    }

    win.run();
}
```

`win.run()` opens the event loop for this single-window app. [Tutorial 10](10-multiwindow.md) covers multiple windows.

## Build and run

Compile this program the same way you compiled the first tutorial, then run the resulting executable. `win.load("ui.yaml")` reads the file by that relative name, so run the program from the directory that contains `ui.yaml`. If you need the build commands, revisit [Tutorial 1](01-hello-fluffy.md#build-and-run).

## Step by step

### 1. Describe the UI

The root of the file is a `Container`, with `x`, `y`, `width`, and `height` giving it a position and size. Inside it, `Vertical:` is the layout entry: it tells the container to stack the following widgets from top to bottom. `spacing: 16` puts 16 pixels between children, and `padding: 40` leaves 40 pixels of empty space around them.

Each entry after the layout creates a child widget. A `Label:` entry becomes a label, a `TextInput:` entry becomes a text field, and a `Button:` entry becomes a button. The indented keys under each entry are its properties.

Property names are written in lowercase snake case, such as `text_size`. Event keys keep their camelCase names, such as `onClick` and `onSubmit`. A misspelled property is ignored, so if a setting seems to do nothing, check its spelling first.

### 2. Register handlers

```cpp
win.dispatcher().registerHandler("login",
    [](const fluffy::EventArgument&) {
        std::printf("Signing in...\n");
    });
```

Every `onClick` or `onSubmit` in the YAML contains a handler name. Here both the username field's `onSubmit` and the button's `onClick` use `"login"`, so either action runs the same callback. The callback receives an `EventArgument` with details about the event; this example ignores it and prints a line.

Register the handler before loading the file. Loading connects names to handlers that already exist; it cannot invent behaviour for a name nobody registered.

### 3. Load the file

```cpp
auto loaded = win.load("ui.yaml");
if (!loaded) {
    std::printf("Failed to load UI: %s\n", loaded.error().c_str());
    return 1;
}
```

`load` returns a status that tells you whether the file was read and parsed. Check it before calling `win.run()`, so a missing or invalid file fails with a clear message instead of opening an empty window. When loading fails, `loaded.error()` returns the message to print.

### 4. Run the window

```cpp
win.run();
```

Once the handler is registered and the file is loaded, `win.run()` opens the event loop. Clicking the button or pressing Enter in the username field calls the `login` handler.

## Exercises

1. Add a second button named "Cancel" with its own handler.
2. Add a horizontal row of buttons at the bottom by nesting a `Container` with a `Horizontal` layout.
3. Change the theme from the C++ side before calling `win.run()`, for example with `win.setTheme(fluffy::Theme::defaultDark())`. (The theme idea is covered in [Tutorial 5](05-icons-and-themes.md).)

## Next tutorial

[Dialogs →](07-dialogs.md)
