# Layouts

A layout is a rule that decides where a container's children sit. Instead of giving every child a rectangle, you give the container a layout and let it arrange the children for you.

## What it's for

A **container** is a widget that holds other widgets. A layout belongs to a container and runs every frame: it measures the children, divides the container's area, and assigns each child a new rectangle.

Fluffy ships three layouts:

- `HorizontalLayout` — children sit side by side in a row. Useful for toolbars and button strips.
- `VerticalLayout` — children sit stacked in a column. Useful for forms and sidebars.
- `GridLayout` — children sit in equal cells (`cols × rows`). Useful for forms, keypads, and dashboards.

A grid fills its cells **row-major**: left to right across the first row, then left to right across the next row, and so on.

Layouts also have **spacing** (empty space between children), **padding** (empty space inside the container, around all children), and **margin** (an inset that shrinks each child's slot). You will see each one when it first appears below.

## A first example

```cpp
#include <fluffy/fluffy.hpp>

int main() {
    fluffy::Window win = fluffy::Window::open("Layouts", 640, 400);
    if (!win) return 1;

    auto& row = win.add<fluffy::Container>();
    row.setBounds(fluffy::Rect{40, 40, 400, 60});
    row.setHorizontal(8.0f);

    auto& a = row.add<fluffy::Button>("A");
    auto& b = row.add<fluffy::Button>("B");
    auto& c = row.add<fluffy::Button>("C");

    win.run();
}
```

`Window::open` creates the window, and `if (!win)` stops early if it could not be opened. `win.add<fluffy::Container>()` creates a container inside the window and returns a reference; `setBounds` gives it a position and size.

`setHorizontal(8.0f)` installs a `HorizontalLayout` with 8 pixels of spacing, which is the empty space between neighboring children. You do not have to create the layout object or call `setLayout()` yourself.

Each `row.add<fluffy::Button>()` appends a child and returns a reference to it. The layout places the three buttons in a row, in the order you added them, and gives each one the same share of the container's height.

## Common tasks

### Stack children vertically

```cpp
auto& col = win.add<fluffy::Container>();
col.setBounds(fluffy::Rect{40, 40, 200, 200});
col.setVertical(8.0f);

col.add<fluffy::Button>("A");
col.add<fluffy::Button>("B");
col.add<fluffy::Button>("C");
```

`setVertical(8.0f)` works the same way as `setHorizontal()`, but stacks the children in a column. The container reference is named `col` so it is easy to tell which direction it lays out.

### Build a grid

A grid divides the container into equal cells. `rows` can stay at `0`, which means "add as many rows as the children need". Set `rows` to a positive number when you want to pin the height of the grid.

```cpp
auto& grid = win.add<fluffy::Container>();
grid.setBounds(fluffy::Rect{40, 40, 360, 180});

auto lo = std::make_unique<fluffy::GridLayout>(3);
lo->setSpacing(8.0f);
lo->setPadding(4.0f);
grid.setLayout(std::move(lo));

for (int i = 1; i <= 6; ++i)
    grid.add<fluffy::Button>("Cell " + std::to_string(i));
```

`std::make_unique<fluffy::GridLayout>(3)` creates a grid layout with 3 columns. The two `setSpacing` / `setPadding` calls configure the layout, and `setLayout` hands ownership of the layout to the container. The loop adds six buttons: with three columns, they fill two rows of three.

For more control, call `setCols(4)` to change the column count or `setRows(2)` to pin the row count on the layout before handing it to `setLayout`.

### Tune spacing, padding and margins

- **Spacing** is the empty space **between** children.
- **Padding** is the empty space **inside** the container, around all children.
- **Margin** is a per-side inset applied on both axes; it shrinks each child's slot.

```cpp
auto& toolbar = win.add<fluffy::Container>();
toolbar.setBounds(fluffy::Rect{40, 40, 400, 60});
toolbar.setHorizontal(16.0f, 8.0f);

fluffy::Layout* lo = toolbar.layout();
lo->setMargin(4.0f);

toolbar.add<fluffy::Button>("Cut");
toolbar.add<fluffy::Button>("Copy");
```

The two values passed to `setHorizontal()` are spacing and padding, in that order. Here the buttons have 16 pixels between them, the container has 8 pixels of padding, and each child's slot is inset by 4 pixels on every side.

The manual form is still available for custom layouts:

```cpp
auto lo = std::make_unique<fluffy::HorizontalLayout>();
lo->setSpacing(16.0f);
lo->setPadding(8.0f);
toolbar.setLayout(std::move(lo));
```

### Align and justify children

A layout has two axes. The **main axis** is the direction in which children are laid out: horizontal for a row, vertical for a column. The **cross axis** is the other direction.

`align()` positions children on the cross axis. `justify()` distributes them along the main axis:

```cpp
fluffy::Layout* lo = toolbar.layout();
lo->align(fluffy::Align::Center);
lo->justify(fluffy::Justify::SpaceBetween);
```

In YAML, use `align` or `align_items` (`start|left|top`, `center|centre|middle`, `end|right|bottom`, case-insensitive) and `justify` or `justify_content` (`start|center|end|space-between|space-evenly`).

You can inspect a layout with `isHorizontal()` and `isVertical()`, and read values back with `spacing()`, `padding()`, `margin()`, `align()`, and `justify()`.

### Use full-window helpers

A window can create a root container that fills the whole window and install a layout on it. This saves you from calculating a `Rect` for the container:

```cpp
auto& col = win.verticalLayout();
col.setPadding(16.0f).setSpacing(8.0f);
col.add<fluffy::Label>("Name");
col.add<fluffy::TextInput>();
```

The helpers are `win.verticalLayout()`, `win.horizontalLayout()`, and `win.gridLayout(cols)`. Each returns a reference to the same full-window container and installs its layout the first time it is called. Pick the one you need; calling another helper later returns the container you already have.

## Defining layouts in YAML

In YAML, the layout is the first map-valued child of `Container`. Its key is the layout name: `Horizontal`, `Vertical`, or `Grid`. The entries inside it are the layout's properties.

```yaml
Container:
  x: 40
  y: 40
  width: 400
  height: 60
  Horizontal:
    spacing: 16
    padding: 8
    align_items: middle
  Button:
    text: "A"
  Button:
    text: "B"
```

Grid uses the same shape with `cols` and `rows`:

```yaml
Container:
  x: 40
  y: 40
  width: 360
  height: 180
  Grid:
    cols: 3
    spacing: 8
    padding: 4
  Button:
    text: "Cell 1"
  Button:
    text: "Cell 2"
```

## Good to know

- If a child has `AutoSize` enabled, the layout asks it for its natural size. Otherwise the layout uses the child's current bounds.
- The container's bounds override any hand-set bounds; the layout owns positioning.
- If a grid is larger than its container, make the container scrollable with a fixed size so scroll indicators can appear:

```cpp
grid.setAutoSize(fluffy::AutoSize::Disabled);
grid.setScrollable(true);
```

`AutoSize::Disabled` keeps the container at its current size instead of growing to fit its children. That lets the content overflow, which is what the scrollbars are for.

## Quick reference

| Property | Type | Description |
|---|---|---|
| `cols` | `int` | Number of columns. |
| `rows` | `int` | Number of rows (`0` = auto from child count). |
| `spacing` | `float` | Gap between cells. |
| `padding` | `float` | Inset inside the container. |

| `Align` | Effect |
|---|---|
| `Start` | Children sit at the top/start |
| `Center` | Children are centered |
| `End` | Children sit at the bottom/end |

| `Justify` | Effect |
|---|---|
| `Start` | Pack at the start (default) |
| `Center` | Pack in the center |
| `End` | Pack at the end |
| `SpaceBetween` | Spread out, no outer gaps |
| `SpaceEvenly` | Spread out with equal outer gaps |

## See also

- [Container](container.md) — the widget that hosts a layout.
- [Widget](widget.md) — `setAutoSize` and `naturalWidth()` / `naturalHeight()`.
