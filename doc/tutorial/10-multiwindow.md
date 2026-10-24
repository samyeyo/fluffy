# Tutorial 10 — Multiwindow with Application

Open two windows at once, give each one its own theme, and keep both alive with a
single application loop.

## What you will build

A light window and a dark window side by side. Each has its own label and a button
that closes only that window. One shared loop keeps both windows responsive until
they are closed.

## What you will learn

- How windows register themselves with `Application`.
- How to keep a separate `Theme` for each window.
- How `Application::get().run()` drives every window.
- How to close one window, or quit the whole application.

## The words you need

- **Application** — the object that keeps track of all open windows and runs their
  shared event loop. You do not create it yourself; `Application::get()` returns
  the one application for the process.
- **Application loop** — a loop that pumps every registered window. `run()` blocks
  until all windows are closed or `quit()` is called. `update()` pumps one frame
  and lets your own code do work between frames.
- **Per-window theme** — colors and sizes belong to a window. Changing one
  window's theme does not change another window's theme.
- **Request close** — ask a window to close. The close button in the title bar
  does the same thing.
- **Layout** — as in [Tutorial 4](04-layouts.md), `verticalLayout()` gives a
  window a container that stacks widgets from top to bottom.

## The full program

Create `main.cpp`:

```cpp
#include <fluffy/application.hpp>
#include <fluffy/button.hpp>
#include <fluffy/label.hpp>
#include <fluffy/theme.hpp>
#include <fluffy/window.hpp>

using namespace fluffy;

int main() {
    Window light = Window::open("Fluffy — Light", 440, 300);
    if (!light) return 1;
    light.setTheme(Theme::defaultLight());

    auto& lightCol = light.verticalLayout();
    lightCol.setPadding(24.0f);
    lightCol.setSpacing(12.0f);

    auto& lightLabel = lightCol.add<Label>("Light window");
    lightLabel.setTextSize(18.0f);

    auto& lightClose = lightCol.add<Button>("Close Light");
    lightClose.onClick([&light] {
        light.requestClose();
    });

    Window dark = Window::open("Fluffy — Dark", 440, 300);
    if (!dark) return 1;
    dark.setTheme(Theme::defaultDark());

    auto& darkCol = dark.verticalLayout();
    darkCol.setPadding(24.0f);
    darkCol.setSpacing(12.0f);

    auto& darkLabel = darkCol.add<Label>("Dark window");
    darkLabel.setTextSize(18.0f);

    auto& darkClose = darkCol.add<Button>("Close Dark");
    darkClose.onClick([&dark] {
        dark.requestClose();
    });

    Application::get().run();
}
```

## Build and run

Build the examples with `./build.sh examples` (or `build.bat examples` on
Windows). The repository ships a multiwindow example; run the binary for your
platform:

```bash
./build/examples/multiwindow/multiwindow
./build/examples/multiwindow/multiwindow.app/Contents/MacOS/multiwindow
```

The first path is for Linux and Windows, the second for macOS.

## Step by step

### 1. Open both windows

```cpp
Window light = Window::open("Fluffy — Light", 440, 300);
if (!light) return 1;

Window dark = Window::open("Fluffy — Dark", 440, 300);
if (!dark) return 1;
```

Each call to `Window::open` registers the new window with `Application`
automatically, so you do not need to construct an `Application` or add the windows
to a list. `if (!light)` and `if (!dark)` check each window as it opens.

After the two calls, `Application::get().windowCount()` is `2`. You can reach a
window by position with `Application::get().window(0)` and
`Application::get().window(1)`, and `isAlive()` tells you whether it is still
open.

### 2. Give each window its own theme

```cpp
light.setTheme(Theme::defaultLight());
dark.setTheme(Theme::defaultDark());
```

A theme belongs to the window, not to the application. That is why the light and
dark windows can coexist in the same process: changing `light` cannot change
`dark`. `Theme::defaultLight()` and `Theme::defaultDark()` return the built-in
themes.

### 3. Add the same content to each window

```cpp
auto& lightCol = light.verticalLayout();
lightCol.setPadding(24.0f);
lightCol.setSpacing(12.0f);

auto& lightLabel = lightCol.add<Label>("Light window");
lightLabel.setTextSize(18.0f);

auto& lightClose = lightCol.add<Button>("Close Light");
lightClose.onClick([&light] {
    light.requestClose();
});
```

`verticalLayout()` is the layout from [Tutorial 4](04-layouts.md): it stacks
widgets from top to bottom. Each window gets its own container, so the same code
shape produces two independent columns. The button callback captures its own
window and calls `requestClose` on it.

### 4. Choose between `run()` and `update()`

```cpp
Application::get().run();
```

`run()` blocks and pumps every registered window until all of them are closed or
`Application::get().quit()` is called. This is the simplest choice when the
application loop is all your program needs to do.

If your own code needs to do work between frames, pump one frame at a time
instead:

```cpp
while (Application::get().update()) {
    // do work between frames
}
```

`update()` returns `true` while at least one window is alive and `quit()` has not
been called. Use `update(delaySeconds)` when you want to choose the time step,
for example `update(1.0f / 60.0f)`.

### 5. Close windows correctly

```cpp
light.requestClose();
```

`requestClose` asks a single window to close. On the next frame that window is
hidden, while `run()` keeps going for the other window. The title-bar close button
does the same thing, so users can close either window directly. To stop the whole
application at once, call `Application::get().quit()`.

## Exercises

1. Add a third window with `Theme::solarized()`.
2. Replace `run()` with `while (Application::get().update()) {}` and print
   `windowCount()` when the loop ends.
3. Add a Quit button to one window that calls `Application::get().quit()`.

## Next

See [Application](../application.md) and [Window](../window.md) for the full
reference. Then continue with [Custom Widget →](11-custom-widget.md).
