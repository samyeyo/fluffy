# Tutorial 7 — Dialogs

Ask the user to confirm an action before you perform it, using a modal dialog and a typed answer.

## What you will build

A window with a Delete button and a status label. Clicking Delete opens a small
confirmation dialog. The label then reports whether the item was deleted or the
action was cancelled. Confirming destructive actions is a small habit that
prevents accidental data loss.

## What you will learn

- What a modal dialog is, and how it returns an answer.
- How to show a dialog with `showDialog`.
- How to read the button the user clicked.
- How to use icons and custom labels in a dialog.

## The words you need

- **Modal dialog** — a small separate window that takes over input until the user
  answers it. While it is open, the main window does not accept clicks. When the
  user chooses a button, the dialog closes and hands the answer back to your
  code.
- **Dialog result** — the answer the dialog returns. With `showDialog` it is a
  `DialogChoice` value: `First`, `Second`, `Third`, or `Dismissed` if the user
  closed the dialog without choosing.
- **Inline content** — as in [Tutorial 5](05-icons-and-themes.md), `Inline` mixes
  an icon and text in one label or button. `$()` is the shorthand that builds it.

## The full program

```cpp
#include <fluffy/button.hpp>
#include <fluffy/icons.hpp>
#include <fluffy/inline.hpp>
#include <fluffy/label.hpp>
#include <fluffy/window.hpp>

int main() {
    fluffy::Window win = fluffy::Window::open("Dialogs", 640, 400);
    if (!win) return 1;

    auto& status = win.add<fluffy::Label>("Nothing deleted yet.");
    status.setBounds(fluffy::Rect{40, 40, 560, 32});

    auto& deleteBtn = win.add<fluffy::Button>(
        fluffy::$(fluffy::Icons::trash << " Delete"));
    deleteBtn.setBounds(fluffy::Rect{40, 120, 160, 40});

    deleteBtn.onClick([&] {
        auto answer = win.showDialog(
            "Confirm delete",
            "Are you sure you want to delete this item?",
            {"Yes", "No"});

        if (answer == fluffy::Window::DialogChoice::First) {
            status.setText("Item deleted.");
        } else {
            status.setText("Delete cancelled.");
        }
    });

    win.run();
}
```

## Build and run

Compile this program the same way you compiled the first tutorial, then run the
resulting executable. If you need the build commands, revisit
[Tutorial 1](01-hello-fluffy.md#build-and-run).

## Step by step

### 1. Add a status label

```cpp
auto& status = win.add<fluffy::Label>("Nothing deleted yet.");
status.setBounds(fluffy::Rect{40, 40, 560, 32});
```

`win.add<fluffy::Label>(...)` creates the label inside the window and returns a
reference to it. We keep that reference in `status` so the callback can change
the text later.

### 2. Add a Delete button with an icon

```cpp
auto& deleteBtn = win.add<fluffy::Button>(
    fluffy::$(fluffy::Icons::trash << " Delete"));
deleteBtn.setBounds(fluffy::Rect{40, 120, 160, 40});
```

`$()` builds `Inline` content, so the button shows the trash icon followed by the
word `" Delete"`. The leading space keeps the icon and the text from touching.
`deleteBtn` is a reference to the real button, ready for a click callback.

### 3. Ask with `showDialog`

```cpp
auto answer = win.showDialog(
    "Confirm delete",
    "Are you sure you want to delete this item?",
    {"Yes", "No"});
```

`showDialog` shows a modal dialog and waits for the user to answer. You pass the
dialog title, the message, and the button labels in order. Because `"Yes"` is the
first label, choosing it returns `DialogChoice::First`; `"No"` returns
`DialogChoice::Second`. Closing the dialog without choosing returns
`DialogChoice::Dismissed`.

### 4. React to the answer

```cpp
if (answer == fluffy::Window::DialogChoice::First) {
    status.setText("Item deleted.");
} else {
    status.setText("Delete cancelled.");
}
```

Only the first choice performs the delete. Everything else, including a dismissed
dialog, is treated as a cancellation. This is what makes the confirmation
meaningful: nothing destructive happens until the user explicitly confirms.

### 5. Use a preset button set

`showDialog` also accepts a preset set of buttons. The typed answer still
indexes the buttons, so `Yes` is `First` and `No` is `Second`:

```cpp
auto answer = win.showDialog(
    "Confirm delete",
    "Are you sure?",
    fluffy::Window::DialogButtons::YesNo);

if (answer == fluffy::Window::DialogChoice::First) {
    status.setText("Item deleted.");
}
```

`showDialog` blocks until the user answers, and a dismissed dialog returns
`DialogChoice::Dismissed`.

### 6. Use icons in the message and buttons

`Inline` content also works for the message and for the buttons:

```cpp
win.showDialog(
    "Warning",
    fluffy::$(fluffy::Icons::alert_triangle << " This cannot be undone."),
    std::vector<fluffy::Inline>{
        fluffy::Inline{} << "OK",
        fluffy::Inline{} << "Cancel"});
```

When the message is `Inline`, pass the buttons as a `std::vector<fluffy::Inline>`
too. The preset-buttons form (`DialogButtons`) is for plain text messages.

## Exercises

1. Show a dialog with custom button labels such as `"Keep"` and `"Delete"`, and
   react to the correct `DialogChoice`.
2. Add an icon to the Yes and No buttons with `Inline`.
3. Ask for confirmation before the window closes.

## Next tutorial

[Data Widgets →](08-data-widgets.md)
