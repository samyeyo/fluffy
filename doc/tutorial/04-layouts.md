# Tutorial 4 — Layouts

Let a `Container` and a layout arrange your widgets for you as you build a small settings panel with vertical and horizontal groups.

## What you will build

A settings panel with a title, a name field, and a save button stacked vertically. Below them, a horizontal row holds Cancel and OK buttons.

## What you will learn

- How to use `Container` to group widgets.
- How `VerticalLayout` and `HorizontalLayout` arrange children.
- How to set spacing and padding.
- How to nest a row inside a column.

## The words you need

- **Container** — a widget that holds other widgets. The container owns its children, and you can give it a layout so it arranges them for you.
- **Layout** — a rule for placing a container's children. `VerticalLayout` stacks them top to bottom, and `HorizontalLayout` places them side by side.
- **Spacing** — the empty gap between children.
- **Padding** — the empty space between the container's edge and its children.
- **Root container** — the full-window container returned by `win.verticalLayout()`. It already has the window's size and a vertical layout.

You can read more about these pieces in the [container](../container.md) and [layouts](../layouts.md) references.

## The full program

```cpp
#include <fluffy/fluffy.hpp>

int main() {
    fluffy::Window win = fluffy::Window::open("Layouts", 640, 400);
    if (!win) return 1;

    auto& root = win.verticalLayout();
    root.setSpacing(16.0f);
    root.setPadding(24.0f);

    auto& title = root.add<fluffy::Label>("Settings");
    auto& name = root.add<fluffy::TextInput>("Your name");
    auto& save = root.add<fluffy::Button>("Save");

    auto& row = root.add<fluffy::Container>();
    row.setHorizontal(12.0f);
    auto& cancel = row.add<fluffy::Button>("Cancel");
    auto& ok = row.add<fluffy::Button>("OK");

    win.run();
}
```

`win.run()` is enough for this single-window app. [Tutorial 10](10-multiwindow.md) covers multiple windows.

## Build and run

Compile this program the same way you compiled the first tutorial, then run the resulting executable. If you need the build commands, revisit [Tutorial 1](01-hello-fluffy.md#build-and-run).

## Step by step

### 1. Create a root container

```cpp
auto& root = win.verticalLayout();
```

`win.verticalLayout()` creates a container that fills the window and gives it a vertical layout. The layout stacks children top to bottom. You do not need to calculate a `Rect` for the container; it already knows the window size.

### 2. Set spacing and padding

```cpp
root.setSpacing(16.0f);
root.setPadding(24.0f);
```

`spacing` is the gap between neighboring children. `padding` is the space inside the container, around all the children. Adjusting these two numbers tunes the whole panel without moving every child by hand.

### 3. Add children to the column

```cpp
auto& title = root.add<fluffy::Label>("Settings");
auto& name = root.add<fluffy::TextInput>("Your name");
auto& save = root.add<fluffy::Button>("Save");
```

`root.add` creates each widget inside the container instead of directly in the window. The container owns these widgets, and the vertical layout gives them their positions and sizes. That is why there is no `setBounds` call: the layout is in charge.

### 4. Nest a horizontal row

```cpp
auto& row = root.add<fluffy::Container>();
row.setHorizontal(12.0f);
auto& cancel = row.add<fluffy::Button>("Cancel");
auto& ok = row.add<fluffy::Button>("OK");
```

A row is itself a `Container`, added as the last child of the vertical column. `setHorizontal(12.0f)` gives it a horizontal layout with 12 pixels between its buttons. The buttons sit side by side, while the row as a whole sits below the save button in the vertical stack.

The row measures its own natural height, so the column makes room for it. `setVertical(12.0f)` works the same way when you want a nested column instead.

### 5. Let the layout own the bounds

The container and its layout assign the geometry for every child. You can use whole numbers as well:

```cpp
root.setPadding(24);
root.setSpacing(16);
```

If you ever need a custom layout, you can still build one yourself and install it on a container with `setLayout`. For most screens, the built-in helpers and nested containers are enough.

## Exercises

1. Add a `Help` button to the Cancel/OK row.
2. Center the title text with `title.setHorizontal(fluffy::Label::Align::Center)`.
3. Give the row its own inner space with `row.setPadding(8.0f)`.

## Next tutorial

[Icons and Themes →](05-icons-and-themes.md)
