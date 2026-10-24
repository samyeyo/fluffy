# Tutorial 2 — Counter

Build a small counter with two buttons — one to increase the count and one to reset it — and see how widgets share state and update each other.

## What you will build

A window that shows a count, a `+1` button, and a `Reset` button. Every click updates the number on screen.

## What you will learn

- How to keep a simple program value outside your widgets.
- How to let a callback use that value.
- How to update a label from a callback.
- Why the label changes without a manual redraw.

## The words you need

- **State** — a value your program remembers while it runs. The count is this tutorial's state, and we keep it in a variable in `main`.
- **Callback** — as in [Tutorial 1](01-hello-fluffy.md), the small function a widget calls when something happens, such as a click.
- **Capture** — when a lambda uses a variable from the code around it. `[&]` captures by reference, so the lambda can read and change the real variable. The `[]` in Tutorial 1 captured nothing.
- **Label text** — a `Label` stores the text it shows. Calling `setText` replaces that text, and the next frame draws the new value.

## The full program

```cpp
#include <string>

#include <fluffy/fluffy.hpp>

int main() {
    fluffy::Window win = fluffy::Window::open("Counter", 640, 400);
    if (!win) return 1;

    int count = 0;

    auto& label = win.add<fluffy::Label>("Count: 0");
    label.setBounds(fluffy::Rect{40, 40, 200, 40});

    auto& inc = win.add<fluffy::Button>("+1");
    inc.setBounds(fluffy::Rect{40, 120, 120, 40});

    auto& reset = win.add<fluffy::Button>("Reset");
    reset.setBounds(fluffy::Rect{180, 120, 120, 40});

    inc.onClick([&] {
        ++count;
        label.setText("Count: " + std::to_string(count));
    });

    reset.onClick([&] {
        count = 0;
        label.setText("Count: 0");
    });

    win.run();
}
```

`win.run()` opens the event loop for this single-window app and returns when the window closes. [Tutorial 10](10-multiwindow.md) shows how to run more than one window at once.

## Build and run

Compile this program the same way you compiled the first tutorial, then run the resulting executable. If you need the build commands, revisit [Tutorial 1](01-hello-fluffy.md#build-and-run).

## Step by step

### 1. Keep state outside the widgets

```cpp
int count = 0;
```

`count` lives in `main`, not inside a label or a button. The buttons change it, and then they update the label to match. Keeping the value in one place makes it easy to see where the number comes from and keeps the widgets focused on showing it.

### 2. Create the widgets

```cpp
auto& label = win.add<fluffy::Label>("Count: 0");
label.setBounds(fluffy::Rect{40, 40, 200, 40});

auto& inc = win.add<fluffy::Button>("+1");
inc.setBounds(fluffy::Rect{40, 120, 120, 40});

auto& reset = win.add<fluffy::Button>("Reset");
reset.setBounds(fluffy::Rect{180, 120, 120, 40});
```

Each widget gets an initial text and explicit bounds. The window owns all three widgets, and the `auto&` references let the callbacks reach them later. The label sits above the two buttons because its `y` is smaller.

### 3. Use the count in a click callback

```cpp
inc.onClick([&] {
    ++count;
    label.setText("Count: " + std::to_string(count));
});
```

`onClick` registers a callback, just like in Tutorial 1. The `[&]` captures `count` and `label` by reference, so the callback changes the same `count` variable and the same label you created above.

The callback first increases `count`, then turns the new number into text with `std::to_string`, then writes that text to the label. This "change the state, then update what shows it" pattern is the heart of the app.

### 4. Update the label

```cpp
label.setText("Count: " + std::to_string(count));
```

`Label::setText` replaces the displayed text. Fluffy draws the window again on the next frame, so you do not need to request a repaint yourself.

### 5. Reset the count

```cpp
reset.onClick([&] {
    count = 0;
    label.setText("Count: 0");
});
```

The reset callback puts the state back to zero and writes the initial text to the label. Both buttons follow the same pattern: change `count`, then update the label that displays it.

## Exercises

1. Add a `-1` button below the other two.
2. Start with the reset button disabled, and enable it whenever `count` is not zero.
3. Center the count text inside the label with `label.setHorizontal(fluffy::Label::Align::Center)`.

## Next tutorial

[Input form →](03-input-form.md)
