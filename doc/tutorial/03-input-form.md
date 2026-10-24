# Tutorial 3 — Input Form

Build a small form with a text field and a submit button, then validate the input and show feedback when the user submits.

## What you will build

A window with a `TextInput`, a status `Label`, and a `Submit` button. You can submit by clicking the button or by pressing Enter in the field.

## What you will learn

- How to create a `TextInput` with a placeholder.
- How to read the current value with `text()`.
- How to listen to `onSubmit` and `onChange`.
- How to show dynamic feedback with a label.

## The words you need

- **Text field** — a widget that accepts typed text. In Fluffy that widget is `TextInput`.
- **Placeholder** — the faint hint shown inside an empty field, such as "Enter your name".
- **Reading text from a widget** — `text()` asks the field for the text it currently holds. It returns a string, and the callback can copy it into a local variable.
- **Submit callback** — a callback that runs when the user presses Enter in a single-line field. It receives the submitted text. As in [Tutorial 1](01-hello-fluffy.md), a callback is the function a widget calls when something happens.
- **Change callback** — a callback that runs after every edit and receives the current text.

## The full program

```cpp
#include <string>

#include <fluffy/fluffy.hpp>

int main() {
    fluffy::Window win = fluffy::Window::open("Input Form", 640, 400);
    if (!win) return 1;

    auto& input = win.add<fluffy::TextInput>("Enter your name");
    input.setBounds(fluffy::Rect{40, 40, 300, 36});

    auto& status = win.add<fluffy::Label>("");
    status.setBounds(fluffy::Rect{40, 120, 560, 32});

    auto& submit = win.add<fluffy::Button>("Submit");
    submit.setBounds(fluffy::Rect{40, 200, 120, 40});

    submit.onClick([&] {
        std::string name = input.text();
        if (name.empty()) {
            status.setText("Name cannot be empty.");
        } else {
            status.setText("Hello, " + name + "!");
        }
    });

    input.onSubmit([&](const std::string& name) {
        if (name.empty()) {
            status.setText("Name cannot be empty.");
        } else {
            status.setText("Hello, " + name + "!");
        }
    });

    win.run();
}
```

`win.run()` opens the event loop for this single-window app. [Tutorial 10](10-multiwindow.md) shows how to run several windows at once.

## Build and run

Compile this program the same way you compiled the first tutorial, then run the resulting executable. If you need the build commands, revisit [Tutorial 1](01-hello-fluffy.md#build-and-run).

## Step by step

### 1. Create a `TextInput`

```cpp
auto& input = win.add<fluffy::TextInput>("Enter your name");
input.setBounds(fluffy::Rect{40, 40, 300, 36});
```

The argument is the placeholder, shown while the field is empty and not focused. `setBounds` gives the field a fixed position and size so it is easy to click and type into.

### 2. Add a status label and a submit button

```cpp
auto& status = win.add<fluffy::Label>("");
status.setBounds(fluffy::Rect{40, 120, 560, 32});

auto& submit = win.add<fluffy::Button>("Submit");
submit.setBounds(fluffy::Rect{40, 200, 120, 40});
```

The status label starts empty and acts as a dedicated area for feedback. Keeping it separate from the input means the user can still see and edit what they typed while the message changes.

### 3. Read the value when the button is clicked

```cpp
submit.onClick([&] {
    std::string name = input.text();
    if (name.empty()) {
        status.setText("Name cannot be empty.");
    } else {
        status.setText("Hello, " + name + "!");
    }
});
```

`text()` returns the field's current content. The callback copies it into `name`, checks whether it is empty, and writes a message to the status label. This is the same click callback pattern you saw in [Tutorial 2](02-counter.md): read, decide, then update a widget.

### 4. Submit on Enter

```cpp
input.onSubmit([&](const std::string& name) {
    if (name.empty()) {
        status.setText("Name cannot be empty.");
    } else {
        status.setText("Hello, " + name + "!");
    }
});
```

`onSubmit` runs when the user presses Enter in a single-line input. The callback receives the submitted text as `name`, so it does not need to call `text()` again.

`onChange` is the other text-field callback. It runs whenever the text changes and receives the new text, which is useful for live feedback such as a character count. Both callbacks are explained in the [text input reference](../textinput.md).

### 5. Show feedback

```cpp
status.setText("Hello, " + name + "!");
```

A second `Label` acts as the status area. Both callbacks use a local named `name`: the click callback fills it from `text()`, and the submit callback receives it from `onSubmit`. Changing the label's text is enough to update the screen; the next frame draws the new message.

## Exercises

1. Add a second `TextInput` for an email address, and read both fields with `text()` in the submit callback.
2. Clear the input after a successful submit with `input.setText("")`.
3. Count how many characters the user typed and display it live with `input.onChange`.

## Next tutorial

[Layouts →](04-layouts.md)
