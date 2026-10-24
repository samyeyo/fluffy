# Tutorial 1 — Hello Fluffy

Build your first Fluffy app: a window with a greeting label and a button that prints a message when you click it.

## What you will build

A small desktop window holding a label and a button. Clicking the button prints a line to the terminal, and the window stays open until you close it.

## What you will learn

- What a window is, and how to open one.
- What a widget is, and how to add one to a window.
- How to place a widget with a rectangle.
- How to make a button react to a click.

## The words you need

A few words come up in every Fluffy program, so here they are before we start.

- **Window** — the operating-system window your app draws into. One program can have one window or several.
- **Widget** — any visible element inside a window: a label, a button, a text field, a slider… Widgets live inside a window, and the window owns them.
- **Bounds** — where a widget sits and how big it is, written as `Rect{x, y, width, height}`. `x` and `y` are the top-left corner; `width` and `height` are the size in pixels.
- **Callback** — a small function you hand to a widget so it can call you back when something happens, such as a click. In C++ you usually write these as lambdas, like `[] { … }`.

You do not need to memorise any of this; each one is introduced again when it appears.

## The full program

Create a file named `main.cpp`:

```cpp
#include <cstdio>

#include <fluffy/fluffy.hpp>

int main() {
    fluffy::Window win = fluffy::Window::open("Hello Fluffy", 640, 400);
    if (!win) return 1;

    auto& label = win.add<fluffy::Label>("Hello, Fluffy!");
    label.setBounds(fluffy::Rect{40, 40, 560, 32});

    auto& btn = win.add<fluffy::Button>("Say hello");
    btn.setBounds(fluffy::Rect{40, 120, 140, 40});
    btn.onClick([] {
        std::printf("Hello from the button!\n");
    });

    win.run();
}
```

That is a complete Fluffy app. The rest of this tutorial walks through it one line at a time.

## Build and run

The repository ships this program as the `helloworld` example. Configure and build the examples with:

```bash
cd /path/to/fluffy
./build.sh examples
```

On Windows, use `build.bat examples` instead. Then run the binary for your platform:

```bash
./build/examples/helloworld/helloworld
./build/examples/helloworld/helloworld.app/Contents/MacOS/helloworld
```

The first path is for Linux and Windows, the second for macOS. You can also compile the program yourself with CMake.

## Step by step

### 1. Open a window

```cpp
fluffy::Window win = fluffy::Window::open("Hello Fluffy", 640, 400);
if (!win) return 1;
```

`Window::open` creates a window with a title (`"Hello Fluffy"`) and a size in pixels (640 wide, 400 tall). It returns the window by value, so `win` is your window from here on.

Opening a window can fail — for example, if the machine has no display. `if (!win)` asks the window whether it is usable: when it is, the condition is false and the program continues; when it is not, we return from `main` with a non-zero code to say something went wrong. Checking once, right after opening, is all you need.

> **Want the why?** `open` is the convenient form for a single window. If you later need fine control over the window's creation settings, the [Window](../window.md) reference shows the alternatives.

### 2. Add a label

```cpp
auto& label = win.add<fluffy::Label>("Hello, Fluffy!");
label.setBounds(fluffy::Rect{40, 40, 560, 32});
```

A label is a widget that shows text. `win.add<fluffy::Label>("Hello, Fluffy!")` creates the label **inside the window** and returns a reference to it.

The `auto&` matters: `label` is that reference, so you work with the real widget rather than a copy. The window owns the widget for its whole life, so you never `delete` it.

`setBounds` says where the label goes: `Rect{40, 40, 560, 32}` puts its top-left corner at (40, 40), 560 pixels wide and 32 tall.

### 3. Add a button

```cpp
auto& btn = win.add<fluffy::Button>("Say hello");
btn.setBounds(fluffy::Rect{40, 120, 140, 40});
```

A button is a widget you can click. It is created and placed the same way as the label, just lower down the window (its `y` is 120) and smaller (140×40).

### 4. React to a click

```cpp
btn.onClick([] {
    std::printf("Hello from the button!\n");
});
```

`onClick` registers a callback — the code to run when the user clicks the button. The `[] { … }` part is a lambda: a tiny unnamed function written right where you need it. This one prints a line to the terminal when it runs.

`onClick` fires when the mouse is pressed **and** released inside the button. Space and Enter do not activate buttons yet.

### 5. Keep the window open

```cpp
win.run();
```

`win.run()` starts the event loop: Fluffy keeps the window alive, listens for clicks and resizes, and calls your callbacks until you close the window. Everything above only sets up the window; `run()` is what makes it live.

For a single window this is all you need. Tutorial 10 shows how to run several windows at once with `Application::get().run()`.

## Exercises

1. Change the window title.
2. Move the label to the top-right corner of the window.
3. Change the label text to something that greets the reader.
4. Add a second button with its own `onClick`.

## Next tutorial

[Counter app →](02-counter.md)
