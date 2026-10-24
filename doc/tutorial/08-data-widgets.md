# Tutorial 8 — Data Widgets

Build a small data panel with a dropdown, a list, tabs, a table, a grid of buttons, and two progress rings.

## What you will build

One window that shows several kinds of data controls at the same time: a combo
box, a selectable list, two tabs, an editable table, a grid of buttons, and two
progress rings. Each control is placed with an explicit `fluffy::Rect`, so you can
see how the pieces fit together before using layouts for the whole window.

## What you will learn

- How selection works in `ComboBox`, `ListView`, and `TabView`.
- How to fill a dropdown and a list with `Inline` items.
- How `TabView` swaps its content when the selected tab changes.
- How a `Table` describes typed columns and rows.
- How to arrange buttons in equal grid cells.
- How a `ProgressRing` shows determinate or indeterminate work.

## The words you need

- **Selection** — a data widget lets the user pick one item. The widget remembers
  the selected index (`0` is the first item) and calls a callback when the
  selection changes.
- **Combo box** — a button that shows the current choice. Clicking it opens a list
  of choices. The Fluffy widget is `ComboBox`.
- **List** — a scrollable column of rows. The Fluffy widget is `ListView`.
- **Tab** — a page label in a row. Choosing a tab swaps the content shown below
  it. The Fluffy widget is `TabView`.
- **Table** — rows and columns of cells, with columns that can be edited as text,
  true/false, or a choice. The Fluffy widget is `Table`.
- **Grid** — a container that arranges its children into equal columns. In Fluffy
  you enable one with `Container::setGrid`.
- **Progress ring** — a circular indicator. A value from `0` to `1` shows progress;
  the value `-1` shows an indeterminate spinner.
- **Inline** — as in [Tutorial 5](05-icons-and-themes.md), `Inline` mixes icons,
  colors, and text in one item.

## The full program

```cpp
#include <cstdio>
#include <string>
#include <vector>

#include <fluffy/button.hpp>
#include <fluffy/combobox.hpp>
#include <fluffy/container.hpp>
#include <fluffy/icons.hpp>
#include <fluffy/label.hpp>
#include <fluffy/listview.hpp>
#include <fluffy/progress_ring.hpp>
#include <fluffy/table.hpp>
#include <fluffy/tabview.hpp>
#include <fluffy/window.hpp>

int main() {
    fluffy::Window win = fluffy::Window::open("Data Demo", 900, 600);
    if (!win) return 1;

    auto& cb = win.add<fluffy::ComboBox>(fluffy::Rect{40, 40, 240, 32});
    cb.setItemsInline({
        fluffy::Inline{} << fluffy::Icons::apple << " Apple",
        fluffy::Inline{} << fluffy::Color{200, 40, 40, 255}
                         << fluffy::Icons::heart << " Cherry",
        fluffy::Inline{} << " Plain"
    });
    cb.setSelectedIndex(1);
    cb.onChange([](int idx, const std::string& value) {
        std::printf("combo %d:%s\n", idx, value.c_str());
    });

    auto& lv = win.add<fluffy::ListView>(fluffy::Rect{40, 100, 240, 120});
    lv.setItemsInline({
        fluffy::Inline{} << fluffy::Icons::star << " Star",
        fluffy::Inline{} << fluffy::Color{40, 120, 220, 255} << "Blue text"
    });
    lv.onSelect([](int idx, const std::string& value) {
        std::printf("list %d:%s\n", idx, value.c_str());
    });

    auto& tv = win.add<fluffy::TabView>(fluffy::Rect{320, 40, 540, 200});
    tv.addTab("Overview", "overview");
    tv.addTab("Details", "details");
    auto& tabContent = *tv.content();
    tabContent.add<fluffy::Label>(
        fluffy::Rect{}, "Pick a tab", fluffy::Label::Align::Left);
    tv.onSelect([&tabContent](int idx, const std::string&) {
        tabContent.removeAllChildren();
        tabContent.add<fluffy::Label>(idx == 0 ? "Overview" : "Details");
    });

    auto& tbl = win.add<fluffy::Table>(fluffy::Rect{40, 400, 820, 150});
    std::vector<fluffy::Table::Column> columns = {
        {"Name", 160, true, fluffy::Table::Column::Kind::Text},
        {"Active", 70, true, fluffy::Table::Column::Kind::Bool},
        {"City", 140, true, fluffy::Table::Column::Kind::Choice},
    };
    columns[2].choices = {"Paris", "London", "Berlin"};
    tbl.setColumns(columns);
    tbl.setHeader(0, fluffy::Inline{} << fluffy::Icons::user << " Name");
    tbl.setRows({{"Alice", "true", "Paris"}, {"Bob", "false", "London"}});

    auto& grid = win.add<fluffy::Container>(fluffy::Rect{320, 270, 400, 100});
    grid.setGrid(3, 8.0f);
    for (int i = 1; i <= 6; ++i) {
        auto& cell = grid.add<fluffy::Button>("Cell " + std::to_string(i));
        cell.setTextSize(14.0f);
    }

    auto& spinner = win.add<fluffy::ProgressRing>(
        fluffy::Rect{780, 270, 48, 48}, -1.0f);
    spinner.setIndeterminate(true);

    auto& progress = win.add<fluffy::ProgressRing>(
        fluffy::Rect{780, 330, 48, 48}, 0.65f);
    progress.setValue(0.65f);

    win.run();
}
```

## Build and run

Compile this program the same way you compiled the first tutorial, then run the
resulting executable. If you need the build commands, revisit
[Tutorial 1](01-hello-fluffy.md#build-and-run).

## Step by step

### 1. Start with a ComboBox

```cpp
auto& cb = win.add<fluffy::ComboBox>(fluffy::Rect{40, 40, 240, 32});
cb.setItemsInline({
    fluffy::Inline{} << fluffy::Icons::apple << " Apple",
    fluffy::Inline{} << fluffy::Color{200, 40, 40, 255}
                     << fluffy::Icons::heart << " Cherry",
    fluffy::Inline{} << " Plain"
});
cb.setSelectedIndex(1);
cb.onChange([](int idx, const std::string& value) {
    std::printf("combo %d:%s\n", idx, value.c_str());
});
```

`setItemsInline` fills the dropdown with `Inline` items, so you can mix icons,
colors, and plain text. `setSelectedIndex(1)` starts on the second item, because
indexes begin at `0`. `onChange` is the selection callback: it receives the new
index and the plain text of the item.

### 2. Add a ListView

```cpp
auto& lv = win.add<fluffy::ListView>(fluffy::Rect{40, 100, 240, 120});
lv.setItemsInline({
    fluffy::Inline{} << fluffy::Icons::star << " Star",
    fluffy::Inline{} << fluffy::Color{40, 120, 220, 255} << "Blue text"
});
lv.onSelect([](int idx, const std::string& value) {
    std::printf("list %d:%s\n", idx, value.c_str());
});
```

`ListView` takes the same `Inline` items as `ComboBox`. It scrolls when the rows
do not fit in its rectangle. `onSelect` runs when the user selects a row and
receives the row's index and text.

### 3. Swap content with a TabView

```cpp
auto& tv = win.add<fluffy::TabView>(fluffy::Rect{320, 40, 540, 200});
tv.addTab("Overview", "overview");
tv.addTab("Details", "details");
auto& tabContent = *tv.content();
tabContent.add<fluffy::Label>(
    fluffy::Rect{}, "Pick a tab", fluffy::Label::Align::Left);
tv.onSelect([&tabContent](int idx, const std::string&) {
    tabContent.removeAllChildren();
    tabContent.add<fluffy::Label>(idx == 0 ? "Overview" : "Details");
});
```

`addTab` adds a header label and an id. The body below the headers lives in a
`Container` that you reach with `tv.content()`. When the selection changes, clear
that container with `removeAllChildren` and add the new content. The first tab is
selected automatically, so the window starts on Overview.

### 4. Build a Table with typed columns

```cpp
std::vector<fluffy::Table::Column> columns = {
    {"Name", 160, true, fluffy::Table::Column::Kind::Text},
    {"Active", 70, true, fluffy::Table::Column::Kind::Bool},
    {"City", 140, true, fluffy::Table::Column::Kind::Choice},
};
columns[2].choices = {"Paris", "London", "Berlin"};
tbl.setColumns(columns);
tbl.setHeader(0, fluffy::Inline{} << fluffy::Icons::user << " Name");
tbl.setRows({{"Alice", "true", "Paris"}, {"Bob", "false", "London"}});
```

Each column describes its header text, width, whether it is editable, and its
`Kind`. `Text` edits text, `Bool` is a true/false cell, and `Choice` lets the user
pick from the column's `choices`. `setHeader` accepts `Inline`, so a header can
carry an icon. `setRows` supplies the cell values as strings; a `Bool` cell uses
`"true"` and `"false"`.

### 5. Arrange buttons in a grid

```cpp
auto& grid = win.add<fluffy::Container>(fluffy::Rect{320, 270, 400, 100});
grid.setGrid(3, 8.0f);
for (int i = 1; i <= 6; ++i) {
    auto& cell = grid.add<fluffy::Button>("Cell " + std::to_string(i));
    cell.setTextSize(14.0f);
}
```

`setGrid(3, 8.0f)` gives the container three equal columns with `8` pixels of
spacing. Adding six buttons produces two rows of three cells, without you
calculating positions. This is the same layout idea as
[Tutorial 4](04-layouts.md), applied inside one rectangle.

### 6. Show progress with ProgressRing

```cpp
auto& spinner = win.add<fluffy::ProgressRing>(
    fluffy::Rect{780, 270, 48, 48}, -1.0f);
spinner.setIndeterminate(true);

auto& progress = win.add<fluffy::ProgressRing>(
    fluffy::Rect{780, 330, 48, 48}, 0.65f);
progress.setValue(0.65f);
```

`-1` makes a ring indeterminate: it spins to show that work is happening, but not
how much is left. A value from `0` to `1` is determinate, so `0.65f` shows about
65 percent. `setIndeterminate` and `setValue` change the mode after creation.

## Exercises

1. Change the combo box's initial selection and watch the index passed to
   `onChange`.
2. Add a fourth `Text` column to the table and fill in a value for both rows.
3. Make a determinate `ProgressRing` follow a `Slider`.

## Next tutorial

[WebView →](09-webview.md)
