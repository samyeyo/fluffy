// examples/textinput/main.cpp

// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

// textinput — demonstration of the multiline TextInput widget.

#include <fluffy/button.hpp>
#include <fluffy/error.hpp>
#include <fluffy/geom.hpp>
#include <fluffy/label.hpp>
#include <fluffy/icons.hpp>
#include <fluffy/inline.hpp>
#include <fluffy/textinput.hpp>
#include <fluffy/theme.hpp>
#include <fluffy/widget.hpp>
#include <fluffy/window.hpp>

namespace {
// ––––––––[ countChars ] counts UTF-8 codepoints -- matches TextInput's per-char cursor math.
int countChars(const std::string& s) {
    int n = 0;
    for (std::size_t i = 0; i < s.size(); ) {
        const auto cp = static_cast<unsigned char>(s[i]);
        const int bytes =
            (cp < 0x80) ? 1 :
            (cp < 0xC0) ? 1 :
            (cp < 0xE0) ? 2 :
            (cp < 0xF0) ? 3 : 4;
        i += static_cast<std::size_t>(bytes);
        ++n;
    }
    return n;
}

// ––––––––[ countLines ] returns the number of user-visible lines (empty -> 0).
int countLines(const std::string& s) {
    if (s.empty()) return 0;
    int n = 1;
    for (char c : s) if (c == '\n') ++n;
    return n;
}
}  // namespace

int main() {
    using namespace fluffy;

    Window win = Window::open("Fluffy — TextInput (multiline)", 800, 600);
    if (!win) return 1;
    win.setTheme(Theme::defaultDark());

    win.add<Label>(
        Rect(32, 24, 736, 32),
        "Fluffy TextInput — Multiline Demo",
        Label::Align::Left, Label::Align::Center)
        .setTextSize(20.0f);

    win.add<Label>(
        Rect(32, 56, 736, 20),
        "Single-line Title fires onSubmit on Enter; multiline Notes never does — use Submit.",
        Label::Align::Left, Label::Align::Center)
        .setTextSize(13.0f);

    // ––––––––[ Title input ] single-line — fires onSubmit when user presses Enter.
    auto& title = win.add<TextInput>(
        Rect(32, 88, 736, 32),
        "Untitled note");
    title.setPrefix($(Icons::pencil << " Title:"));

    // ––––––––[ multiline ] multi-line — Enter inserts a newline; onSubmit does not fire.
    auto& notes = win.add<TextInput>(
        Rect(32, 132, 736, 200),
        "Start writing here…");
    notes.setMultiline(true);
    notes.setPrefix($(Icons::file_text << " Notes:"));
    notes.setText(
        "Fluffy is a small, modern C++ GUI engine.\n"
        "It builds on NanoVG and GLFW and renders\n"
        "everything through one OpenGL framebuffer.\n"
        "\n"
        "Try editing this text — the stats below\n"
        "will update as you type.");

    auto& stats = win.add<Label>(
        Rect(32, 348, 736, 22),
        "0 lines   •   0 characters",
        Label::Align::Left, Label::Align::Center);
    stats.setTextSize(13.0f);

    auto updateStats = [&stats](const std::string& txt) {
        stats.setText(
            std::to_string(countLines(txt)) + " lines   •   " +
            std::to_string(countChars(txt)) + " characters");
    };
    notes.setOnChange(updateStats);

    auto& submit = win.add<Button>(
        Rect( 32, 388, 200, 40),
        $(Icons::check << " Submit"));
    auto& clear = win.add<Button>(
        Rect(244, 388, 120, 40),
        $(Icons::trash << " Clear"));

    win.add<Label>(
        Rect(32, 444, 736, 20),
        "───  Latest commit  ───",
        Label::Align::Left, Label::Align::Center)
        .setTextSize(13.0f);

    auto& echo = win.add<Label>(
        Rect(32, 468, 736, 60),
        "(empty — press Enter on Title, or click Submit on Notes)",
        Label::Align::Left, Label::Align::Center);
    echo.setTextSize(13.0f);

    submit.setOnClick([&notes, &echo]() {
        echo.setText(notes.text().empty()
            ? std::string("(Notes were empty at submit)")
            : notes.text());
    });
    clear.setOnClick([&notes, &echo]() {
        notes.setText("");
        echo.setText("(Notes cleared)");
    });

    // ––––––––[ setOnSubmit ] on the single-line Title — fires when user presses Enter.
    title.setOnSubmit([&echo](const std::string& t) {
        echo.setText(t.empty()
            ? std::string("(Title submitted empty)")
            : std::string("[Title] ") + t);
    });

    win.add<Button>(
        Rect( 32, 548, 120, 36),
        "Light")
        .setOnClick([&win]() { win.setTheme(Theme::defaultLight()); });
    win.add<Button>(
        Rect(164, 548, 120, 36),
        "Dark")
        .setOnClick([&win]() { win.setTheme(Theme::defaultDark()); });
    win.add<Button>(
        Rect(296, 548, 140, 36),
        "Solarized")
        .setOnClick([&win]() { win.setTheme(Theme::solarized()); });

    updateStats(notes.text());

    win.run();
    return 0;
}
