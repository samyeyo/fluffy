# Container

A `Container` is a widget that holds other widgets. You use it to group widgets, arrange them in a row, column, or grid, or make a region that scrolls.

## What it's for

A container is a widget whose job is to own and organize other widgets. The widgets inside it are called its **children**. You create the children once, and the container keeps them alive: when the container is destroyed, its children go with it.

Children are drawn and receive input through their container, so a button inside a container still behaves like a button. You can also put containers inside containers when you need groups within groups.

A container can have a **layout**. A layout is a rule that decides where the container's children sit and how much space they get. Without a layout, children keep the bounds you give them. With a layout, the container positions the children for you every frame.

If you make a container **scrollable**, it clips anything that runs past its edges, shows scroll indicators, and lets the mouse wheel move the content. This is useful when the content is taller or wider than the visible area.

## A first example

```cpp
#include <fluffy/fluffy.hpp>

int main() {
    fluffy::Window win = fluffy::Window::open("Container", 640, 400);
    if (!win) return 1;

    auto& box = win.add<fluffy::Container>();
    box.setBounds(fluffy::Rect{40, 40, 400, 200});
    box.setVertical(4.0f, 8.0f);

    auto& first = box.add<fluffy::Label>("First");
    box.add<fluffy::Label>("Second");

    win.run();
}
```

`Window::open` creates the window, and `if (!win)` stops early if it could not be opened. `win.add<fluffy::Container>()` creates a container inside the window and returns a reference to it. The `auto&` matters: `box` is that reference, so you work with the real container rather than a copy.

`setBounds` gives the container its position and size. `Rect{40, 40, 400, 200}` puts its top-left corner at (40, 40), with a width of 400 and a height of 200 pixels.

`setVertical(4.0f, 8.0f)` installs a vertical layout. The first number is the **spacing**: 4 pixels between children. The second is the **padding**: 8 pixels of empty space inside the container, around all children.

`box.add<fluffy::Label>("First")` adds a child. The layout places it for you, so children do not need their own `setBounds` calls. The `add` call returns a reference too, which you can keep when you need to talk to the child again.

## Common tasks

### Arrange children with a layout

Once a container has a layout, the layout assigns child bounds every frame. Children may start with an empty `Rect{}`; the layout fills it in.

`setHorizontal()`, `setVertical()`, and `setGrid()` install a layout in one call. The spacing and padding shortcuts are chainable, and `align()` and `justify()` forward to the layout:

```cpp
box.setPadding(8.0f)
   .setSpacing(4.0f)
   .align(fluffy::Align::Center)
   .justify(fluffy::Justify::Start);

fluffy::Layout* lo = box.layout();
```

`align` positions children on the layout's cross axis; `justify` distributes them along its main axis. For a vertical layout the main axis is vertical, so `align` moves children left and right. For a horizontal layout it is the other way around. The layout itself is a `HorizontalLayout`, `VerticalLayout`, or `GridLayout`; the [Layouts](layouts.md) page explains each one.

### Add nested groups

`addRow()` creates a child container with a horizontal layout, and `addColumn()` creates one with a vertical layout. Both have automatic height and return a pointer to the new container.

```cpp
auto* row = box.addRow();
auto* col = box.addColumn();

row->add<fluffy::Label>("Name");
col->add<fluffy::Label>("Value");
```

Nesting is useful for layouts such as a label and a value side by side inside a vertical form.

### Inspect and remove children

```cpp
std::size_t n = box.childCount();
fluffy::Widget* first = box.child(0);
box.removeAllChildren();
```

`childCount()` returns how many children the container has. `child(0)` returns a pointer to the first child, or `nullptr` when the index is out of range. `removeAllChildren()` destroys every child.

### Make a container scrollable

A scrollable container keeps a **scroll offset**: how far its content has been moved, in pixels. The offset is clamped so you cannot scroll past the content.

```cpp
box.setScrollable(true);
bool s = box.isScrollable();
float sx = box.scrollX();
float sy = box.scrollY();
box.setScrollX(0.0f).setScrollY(40.0f);
```

`setScrollable(true)` turns on clipping, indicators, and wheel scrolling. `scrollX()` and `scrollY()` read the current offsets, and `setScrollX()` / `setScrollY()` change them. The setters are chainable and clamp their values.

For a scrollable container, keep `setAutoSize(fluffy::AutoSize::Disabled)` so the container stays at a fixed size and its content can overflow.

### Define a container in YAML

YAML is a text format for describing a widget tree. A `Container:` entry holds its own geometry, then a layout key, then its children:

```yaml
Container:
  x: 40
  y: 40
  width: 400
  height: 200
  scrollable: true
  Vertical:
    spacing: 8
    padding: 8
  Label: { text: "First" }
  Label: { text: "Second" }
```

`x`, `y`, `width`, and `height` are the same rectangle you would pass to `setBounds`. `scrollable` matches `setScrollable`. `Vertical` is the layout key; `spacing` and `padding` configure it. Each `Label` entry becomes a child.

## Good to know

- A container fills its parent and reports that it is a container, so parents know to pass events and drawing down to its children.
- The window pointer and focus clearing are passed down through the whole tree, so nested widgets keep working normally.
- You can walk every child with `forEachChild()`.
- A container supports the usual widget properties in YAML and can size itself from its children's natural sizes.
- The layout repositions children every frame. Child rectangles may be empty and will be assigned by the layout.
- A container has no events of its own. When it is scrollable, it uses the mouse wheel to adjust `scrollX` / `scrollY`.

## Quick reference

| Property | Type | Description |
|---|---|---|
| `scrollable` | `bool` | `true` clips children to bounds and enables wheel scrolling. Default `false`. |
| `scrollX` / `scrollY` | `float` | Current scroll offset (clamped). |
| `layout` | `unique_ptr<Layout>` | `HorizontalLayout`, `VerticalLayout`, or `GridLayout`. |

Plus `Widget` properties: `bounds`, `visible`, `enabled`, `autoSize`.

```cpp
auto& box = win.add<fluffy::Container>();
box.setBounds(fluffy::Rect{40, 40, 400, 200});
box.setVertical(4.0f, 8.0f);
fluffy::Layout* lo = box.layout();

box.setPadding(8.0f).setSpacing(4.0f)
   .align(fluffy::Align::Center).justify(fluffy::Justify::Start);

box.add<fluffy::Label>("First");
auto& lbl = box.add<fluffy::Label>("Second");
auto* row = box.addRow();
auto* col = box.addColumn();
std::size_t n = box.childCount();
fluffy::Widget* c = box.child(0);
box.removeAllChildren();

box.setScrollable(true);
bool s = box.isScrollable();
float sx = box.scrollX();
float sy = box.scrollY();
box.setScrollX(0.0f).setScrollY(40.0f);
```

## See also

- [Layouts](layouts.md) — `HorizontalLayout`, `VerticalLayout` and `GridLayout`.
- [Widget](widget.md)
