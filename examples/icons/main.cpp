// examples/icons/main.cpp

// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

// icons — visual catalogue of Lucide SVG icons with theme switching.

#include <fluffy/button.hpp>
#include <fluffy/geom.hpp>
#include <fluffy/label.hpp>
#include <fluffy/icons.hpp>
#include <fluffy/theme.hpp>
#include <fluffy/widget.hpp>
#include <fluffy/window.hpp>
int main() {
    using namespace fluffy;
    Window win = Window::open("Fluffy - Lucide Icon Showcase", 1080, 940);
    if (!win) return 1;
    win.add<Label>(
        Rect(60, 24, 480, 44),
        "Lucide Icon Showcase",
        Label::Align::Left);
    auto& theme_status = win.add<Label>(
        Rect(60, 64, 480, 24),
        "Current theme: light",
        Label::Align::Left);
    win.add<Label>(
        Rect(60, 92, 1040, 22),
        $(Style{ .fg = Color::fromHex(0xFFB6B6BC) }
            << "Inline colours: "
            << Color::fromHex(0xFF4F8DFF) << Icons::rocket    
            << " "
            << Color::fromHex(0xFF45C055) << Icons::check      
            << " "
            << Color::fromHex(0xFFE0506E) << Icons::heart      
            << " "
            << Color::fromHex(0xFFE8B339) << Icons::star       
            << " "
            << Color::fromHex(0xFFA050E0) << Icons::zap        
            << " "
            << Color::fromHex(0xFFFF8A3A) << Icons::flame      
            << Style{}
            << "."),
        Label::Align::Left);
    win.add<Button>(Rect(580, 32, 140, 40), "Light")
        .setOnClick([&win, &theme_status]() {
            win.setTheme(Theme::defaultLight());
            theme_status.setText("Current theme: light");
        });
    win.add<Button>(Rect(740, 32, 140, 40), "Dark")
        .setOnClick([&win, &theme_status]() {
            win.setTheme(Theme::defaultDark());
            theme_status.setText("Current theme: dark");
        });
    win.add<Button>(Rect(900, 32, 140, 40), "Solarized")
        .setOnClick([&win, &theme_status]() {
            win.setTheme(Theme::solarized());
            theme_status.setText("Current theme: solarized");
        });
    const float start_x        = 60.0f;
    const float col_width      = 160.0f;
    const float row_height     = 36.0f;
    const float right_margin   = 60.0f + 6.0f * col_width; 
    const float header_padding = 30.0f;
    const float body_top_y     = 120.0f;
    float cursor_x = start_x;
    float cursor_y = body_top_y;
    auto add_icon = [&](const Icons::SvgIconRef& icon, const char* name) {
        win.add<Label>(
            Rect(cursor_x, cursor_y, col_width, row_height),
            $(icon << " " << name),
            Label::Align::Left);
        cursor_x += col_width;
        if (cursor_x + col_width > right_margin) {
            cursor_x = start_x;
            cursor_y += row_height;
        }
    };
    auto add_header = [&](const char* title) {
        const bool was_partial_row = (cursor_x != start_x);
        cursor_x  = start_x;
        if (was_partial_row) cursor_y += row_height;
        cursor_y += header_padding;
        win.add<Label>(
            Rect(cursor_x, cursor_y, 480, row_height),
            title,
            Label::Align::Left);
        cursor_y += row_height;
    };
    add_header("UI & Navigation");
    add_icon(Icons::bell,     "bell");
    add_icon(Icons::calendar, "calendar");
    add_icon(Icons::camera,   "camera");
    add_icon(Icons::clock_4,  "clock_4");
    add_icon(Icons::cog,      "cog");
    add_icon(Icons::compass,  "compass");
    add_header("Actions & State");
    add_icon(Icons::archive,   "archive");
    add_icon(Icons::award,     "award");
    add_icon(Icons::bug,       "bug");
    add_icon(Icons::check,     "check");
    add_icon(Icons::clipboard, "clipboard");
    add_icon(Icons::cloud,     "cloud");
    add_header("Objects & Forms");
    add_icon(Icons::battery,   "battery");
    add_icon(Icons::book,      "book");
    add_icon(Icons::box,       "box");
    add_icon(Icons::briefcase, "briefcase");
    add_icon(Icons::building,  "building");
    add_icon(Icons::car,       "car");
    add_header("Arrows & Layout");
    add_icon(Icons::arrow_down,  "arrow_down");
    add_icon(Icons::arrow_left,  "arrow_left");
    add_icon(Icons::arrow_right, "arrow_right");
    add_icon(Icons::arrow_up,    "arrow_up");
    add_icon(Icons::chevron_down, "chevron_down");
    add_header("Tech & Symbols");
    add_icon(Icons::activity,  "activity");
    add_icon(Icons::anchor,    "anchor");
    add_icon(Icons::bluetooth, "bluetooth");
    add_icon(Icons::bot,       "bot");
    add_icon(Icons::circle,    "circle");
    add_header("Late Alphabet (W..Z)");
    add_icon(Icons::warehouse,  "warehouse");
    add_icon(Icons::wifi,       "wifi");
    add_icon(Icons::wrench,     "wrench");
    add_icon(Icons::x,          "x");
    add_icon(Icons::zap,        "zap");
    win.run();
    return 0;
}

