# Welcome to Fluffy

Fluffy is a small C++ library for building desktop apps with windows, buttons, labels, text inputs, and layouts, and it is intentionally tiny.

New here? Start with [Installing Fluffy](install.md) to download, build, and install the library, then come back for the API tour.

## What it's for

Use Fluffy when you want to build a small GUI app without learning a massive framework. You can learn the whole API in an afternoon, and the same few building blocks cover both code-driven and declarative interfaces.

A few words come up in every Fluffy program, so here they are before we start:

- **Window** — the operating-system window your app draws into. It owns everything inside it.
- **Widget** — any visible element in a window: a label, a button, a text field, an image… Widgets live inside a window, and the window owns them.
- **Callback** — a small function you hand to a widget so it can call you back when something happens, such as a click. In C++ you usually write these as lambdas, like `[] { … }`.

## A first example

A window with a label and a button that prints “Hello!” when you click it:

```cpp
#include <cstdio>

#include <fluffy/fluffy.hpp>

int main() {
    fluffy::Window win = fluffy::Window::open("My App", 640, 400);
    if (!win) return 1;

    auto& label = win.add<fluffy::Label>("Hello, Fluffy!");
    label.setBounds(fluffy::Rect{40, 40, 560, 32});

    auto& btn = win.add<fluffy::Button>("Say hello");
    btn.setBounds(fluffy::Rect{40, 120, 120, 40});
    btn.onClick([] {
        std::printf("Hello!\n");
    });

    win.run();
}
```

That is almost all there is to a Fluffy program: you open a window, add widgets, set callbacks, and run. `Window::open` gives you the window, and `if (!win)` checks that it opened. Each widget is added with `add<...>()`, which returns a reference to the widget the window owns.

For two windows, open both and then call `Application::get().run()` — each window keeps its own `Theme` (see [Application](application.md)).

If you are brand new, [Tutorial 1 — Hello Fluffy](tutorial/01-hello-fluffy.md) walks through this exact program line by line.

## Reading path

New here? Read in this order:

1. **[Installing Fluffy](install.md)** — build, install, and link Fluffy from your own project.
2. **[Geometry & colors](geom.md)** — how to say “put a box here” and “paint it blue”.
3. **[Window](window.md)** — the top-level window, main loop, and themes.
4. **[Widget basics](widget.md)** — what every widget can do (move, hide, resize, focus).
5. **[Label](label.md)** · **[Button](button.md)** · **[TextInput](textinput.md)** — the three everyday widgets.
6. **[Container](container.md)** and **[Layouts](layouts.md)** — grouping widgets and arranging them automatically.
7. **[Image](image.md)** — displaying raster images with platform-native decoders.
8. **[Toggles](checkbox.md)** — **[Checkbox](checkbox.md)**, **[RadioButton](radiobutton.md)**, **[Switch](switch.md)**, **[Slider](slider.md)**, **[ColorPicker](color_picker.md)**.
9. **[Data](combobox.md)** — **[ComboBox](combobox.md)**, **[ListView](listview.md)**, **[TabView](tabview.md)**, **[Table](table.md)**, **[ProgressRing](progress_ring.md)**.
10. **[Overlays](tooltip.md)** — **[Tooltip](tooltip.md)**, **[Menu](menu.md)** / **[Nav](nav.md)**, **[WebView](webview.md)**.

After that, the fun parts:

- **[Inline](inline.md)** — mix text and icons in one label or button (now also in `ComboBox`/`ListView`/`Tooltip`/`Menu`).
- **[Icons](icons.md)** — the built-in icon catalog.
- **[Theme](theme.md)** — light mode, dark mode, or your own colors.
- **[Dialogs](dialog.md)** — show yes/no popups with one line of code.
- **[YAML](yaml.md)** — describe your whole UI in a file.
- **Scrollable containers** — clip overflow and show scroll indicators.
- **Word wrapping** — labels wrap text at word boundaries automatically.
- **Dropdown menus** — buttons that open vertical menus on click.
