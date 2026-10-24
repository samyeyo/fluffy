# Application

`fluffy::Application` is the global window manager: a lazy singleton, one per process, that keeps the list of live `Window`s and runs the global event loop.

## What it's for

Use `Application` when your app has more than one window, or when you want to pump frames yourself for games, benchmarks, or headless rendering. For a single window you do not need it at all — `win.run()` is enough.

Every new `Window` auto-registers with `Application::get()` on success and unregisters when it is destroyed. `Theme` stays **per-window** (`Window::Config.theme`, `Window::theme()`, `Window::setTheme()`). `Application` does not hold a theme.

## A first example

```cpp
#include <fluffy/application.hpp>
#include <fluffy/label.hpp>
#include <fluffy/window.hpp>

int main() {
    fluffy::Window w1 = fluffy::Window::open("Hello", 720, 400);
    fluffy::Window w2 = fluffy::Window::open("Palette", 400, 300);
    if (!w1 || !w2) return 1;

    w2.setTheme(fluffy::Theme::solarized());

    w1.verticalLayout().add<fluffy::Label>("Hello from Fluffy!");
    w2.verticalLayout().add<fluffy::Label>("Second window");

    // Blocks until all windows are closed or quit() is called.
    // Closing one window hides it; the app stays alive while any window is alive.
    fluffy::Application::get().run();
    return 0;
}
```

Both windows register themselves as they are created. `Window::open` gives you each window; `if (!w1 || !w2)` checks that both opened. The global loop keeps running while at least one window is alive, and closing one window does not end the app.

If you need full control over how a window is created, `Window::create(Config)` takes a `Config` value and returns a `Result<Window>` — a small box holding either the window or an error message. Check it before using `.value()`; if a failure leaves you with nothing sensible to do, `.unwrap()` returns the window on success and otherwise prints the error and stops the program. See [Window](window.md).

## Common tasks

### Create windows that register themselves

You create windows the usual way; they register automatically:

```cpp
#include <fluffy/application.hpp>
#include <fluffy/window.hpp>

fluffy::Window w1 = fluffy::Window::open("A", 800, 600);
if (!w1) return 1;

auto result = fluffy::Window::create({.title = "B", .width = 640, .height = 400});
if (!result) return 1;
fluffy::Window& w2 = result.value();

w2.setTheme(fluffy::Theme::defaultDark()); // per-window theme kept
```

Moving a `Window` and destroying it update the registry, so the set of live windows stays correct. `Window` objects on the stack remain owned by you; `Application` only holds pointers to them and never deletes them.

### Run the global loop

```cpp
fluffy::Application::get().run();
```

`Application::get().run()` visits every live window: it checks for new input once, updates hover and animations, and draws the window. It exits when no windows are left, when every window reports that it is closed, or when `quit()` was called.

Single-window compatibility is kept — `win.run()` still blocks that window. If more than one window is registered, `Window::run()` delegates to `Application::get().run()`.

### Pump frames manually with `update`

For games, benchmarks, or headless rendering, pump one frame yourself:

```cpp
// explicit dt seconds, returns bool continue?
bool keep = fluffy::Application::get().update(1.0f/60.0f);
while (keep) {
    // game logic
    keep = fluffy::Application::get().update(1.0f/60.0f);
}

// auto dt (computed from the elapsed time, clamped 0-0.05)
while (fluffy::Application::get().update()) {}
```

- `bool update(float delaySeconds)` — `delaySeconds` is seconds to advance `tick` animations (`SmoothAnim`). Clamped `0-0.05`. Returns `true` while any window is alive and `quit()` not called.
- `bool update()` — overload with the delay computed internally (first frame `0.016f`).

`update()` also hides operating-system windows that were just closed, so a closed dark window disappears instead of freezing.

### Manage windows and quit

```cpp
auto* app = fluffy::Application::tryGet(); // nullptr before first Window
size_t n = fluffy::Application::get().windowCount();

if (n > 0) {
    fluffy::Window* w = fluffy::Application::get().window(0);
    bool alive = w->isAlive();
    GLFWwindow* gw = w->glfwWindow(); // for native interop (advanced)
    w->requestClose();
}

fluffy::Application::get().quit();
```

`tryGet()` returns `nullptr` before the first `Window`, so it never creates the singleton by accident. `isAlive()`, `glfwWindow()`, `syncSize()`, `doHoverTick(dt)`, and `renderFrame()` are public helpers for `Application`; you normally don't call them directly.

## Good to know

- `Application` is a static lazy singleton: one per process.
- `Theme` is per-window. `Application` does not hold a theme.
- `Window` objects on the stack remain owned by you; `Application` only holds raw pointers.
- Closing one window hides it while the app stays alive as long as any window is alive.
- `update()` with no argument computes the delay internally; the first frame uses `0.016f`.
- On WebAssembly, `Application` drives the browser's main loop instead of a native one, calling `update()` for each canvas.

## Example

See `examples/multiwindow/main.cpp` — two windows (light and dark per-window themes), each with `verticalLayout`, using `Application::get().run()`. `Close` hides one window, and `Quit App` calls `Application::get().quit()`.

## Quick reference

| API | What it does |
|---|---|
| `Application::get()` | returns the static lazy singleton |
| `Application::tryGet()` | returns `nullptr` before the first `Window` |
| `run()` | pumps every live window until all are closed or `quit()` is called |
| `update(float delaySeconds)` | advances one frame; `delaySeconds` advances `tick` animations and is clamped `0-0.05`; returns `true` while any window is alive and `quit()` not called |
| `update()` | advances one frame with `delay` computed internally (`glfwGetTime()-lastTime_`, first frame `0.016f`) |
| `windowCount()` | number of registered windows |
| `window(i)` | returns a `Window*` by index |
| `quit()` | closes all windows and makes `run()`/`update()` return `false` |
| `Window::run()` | blocks a single window; delegates to `Application::get().run()` when more than one window is registered |
| `Window::isAlive()` | `!glfwWindowShouldClose && !should_close_` |
| `Window::glfwWindow()` | returns the `GLFWwindow*` for native interop |
| `Window::requestClose()` | sets `should_close_` and calls `glfwSetWindowShouldClose` |
| `Window::syncSize()`, `doHoverTick(dt)`, `renderFrame()` | public helpers you normally don't call directly |

## See also

- [Window](window.md) — per-window `Theme`, dialogs, popups.
- [Widget](widget.md) — what every widget can do.
