// examples/showcase/main.cpp

// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

// Showcase — all Fluffy widgets in their natural size via autoSize + 2‑column layout.

#include <fluffy/button.hpp>
#include <fluffy/checkbox.hpp>
#include <fluffy/color_picker.hpp>
#include <fluffy/container.hpp>
#include <fluffy/error.hpp>
#include <fluffy/geom.hpp>
#include <fluffy/icons.hpp>
#include <fluffy/inline.hpp>
#include <fluffy/label.hpp>
#include <fluffy/layout.hpp>
#include <fluffy/radiobutton.hpp>
#include <fluffy/slider.hpp>
#include <fluffy/switch.hpp>
#include <fluffy/textinput.hpp>
#include <fluffy/theme.hpp>
#include <fluffy/widget.hpp>
#include <fluffy/window.hpp>
#include <GLFW/glfw3.h>
#include <string>

int main() {
    using namespace fluffy;

    Window::Config cfg;
    cfg.title     = "Fluffy — Widget Showcase";
    cfg.width     = 1000;
    cfg.height    = 720;
    cfg.min_width  = 680;
    cfg.min_height = 480;
    cfg.theme     = Theme::defaultLight();
    auto win_or_err = Window::create(cfg);
    if (!win_or_err) {
        return 1;
    }
    Window& win = win_or_err.value();

    // Material Dark theme inspired by VS Code's Dark+ theme
    static constexpr const char* kMaterialYaml = R"(
name: material-dark
font_family: default
colors:
  background: "#FF1E1E1E"
  surface: "#FF252526"
  on_surface: "#FFD4D4D4"
  on_surface_mute: "#FFA6A6A6"
  accent: "#FF8F00"
  accent_pressed: "#FF7200"
  border: "#FF303031"
  disabled: "#FF3A3D41"
metrics:
  corner_radius: 2
  border_width: 1
  text_size: 13
  button_padding_x: 14
  button_padding_y: 8
  input_padding_x: 8
  input_padding_y: 6
  input_min_height: 28
  toggle_size: 16
  slider_thumb_size: 14
shadows:
  shadow:
    color: "#66000000"
    blur: 8
    offset_x: 0
    offset_y: 1
  shadow_hover:
    color: "#80000000"
    blur: 12
    offset_x: 0
    offset_y: 2
  shadow_pressed:
    color: "#4D000000"
    blur: 4
    offset_x: 0
    offset_y: 0
animation:
  hover_tau: 0.10
  press_tau: 0.05
)";
    Theme materialTheme = Theme::fromYaml(kMaterialYaml);

    // ── Root: full-window container with horizontal split ────────────
    auto& root = win.add<Container>(Rect(0.0f, 0.0f, 720.0f, 640.0f));
    root.setHorizontal(12.0f, 12.0f);

    // Helper: create a column (Container with VerticalLayout, auto-height)
    auto add_column = [&]() -> Container* {
        auto& col = root.add<Container>();
        col.setVertical(6.0f);
        col.setAutoSize(AutoSize::Height);
        return &col;
    };

    // Helper: add a section heading to a column
    auto add_heading = [&](Container* col, const char* text) {
        auto lbl = std::make_unique<Label>(Rect{}, std::string(text), Label::Align::Left);
        lbl->setTextSize(15.0f);
        col->addChild(std::move(lbl));
    };

    // Helper: add a row (Container with HorizontalLayout, auto-height)
    auto add_row = [&]() -> std::unique_ptr<Container> {
        auto row = std::make_unique<Container>();
        row->setHorizontal(6.0f);
        row->setAutoSize(AutoSize::Height);
        return row;
    };

    // Helper: add a small vertical spacer to a column
    auto add_spacer = [&](Container* col) {
        auto s = std::make_unique<Label>(Rect{}, std::string(""));
        s->setTextSize(4.0f);
        s->setAutoSize(AutoSize::Height);
        col->addChild(std::move(s));
    };

    // ────────────────────────────────────────────────────────────────────
    //  Column 1 — Labels & Buttons
    // ────────────────────────────────────────────────────────────────────
    auto* c1 = add_column();

    add_heading(c1, "Labels");
    // Plain short text
    {
        auto lbl = std::make_unique<Label>(Rect{}, "Short label", Label::Align::Left);
        c1->addChild(std::move(lbl));
    }
    // Longer text
    {
        auto lbl = std::make_unique<Label>(Rect{}, "A longer label that demonstrates natural width", Label::Align::Left);
        c1->addChild(std::move(lbl));
    }
    // Inline with icon
    {
        auto lbl = std::make_unique<Label>(Rect{}, $(Icons::check << " Done"), Label::Align::Left);
        c1->addChild(std::move(lbl));
    }
    // Icon-only label
    {
        auto lbl = std::make_unique<Label>(Rect{}, $(Icons::bell << " Notifications"), Label::Align::Left);
        c1->addChild(std::move(lbl));
    }

    add_spacer(c1);

    add_heading(c1, "Buttons");
    // Row 1: text + icon+text
    {
        auto row = add_row();
        {
            auto btn = std::make_unique<Button>(Rect{}, "Click Me");
            row->addChild(std::move(btn));
        }
        {
            auto btn = std::make_unique<Button>(Rect{}, $(Icons::send << " Send"));
            row->addChild(std::move(btn));
        }
        c1->addChild(std::move(row));
    }
    // Row 2: icon-only + disabled + disabled-with-icon
    {
        auto row = add_row();
        {
            auto btn = std::make_unique<Button>(Rect{}, Inline{} << Icons::cog);
            row->addChild(std::move(btn));
        }
        {
            auto btn = std::make_unique<Button>(Rect{}, "Disabled");
            btn->setEnabled(false);
            row->addChild(std::move(btn));
        }
        {
            auto btn = std::make_unique<Button>(Rect{}, $(Icons::x << " Unavailable"));
            btn->setEnabled(false);
            row->addChild(std::move(btn));
        }
        c1->addChild(std::move(row));
    }
    // Row 3: theme switcher
    {
        auto row = add_row();
        auto add_btn = [&row](const char* label, auto&& cb) {
            auto btn = std::make_unique<Button>(Rect{}, std::string(label));
            btn->setOnClick(std::move(cb));
            row->addChild(std::move(btn));
        };
        add_btn("Light",     [&win]() { win.setTheme(Theme::defaultLight()); });
        add_btn("Dark",      [&win]() { win.setTheme(Theme::defaultDark()); });
        add_btn("Solarized", [&win]() { win.setTheme(Theme::solarized()); });
        add_btn("Material", [&win, &materialTheme]() { win.setTheme(materialTheme); });
        c1->addChild(std::move(row));
    }

    add_spacer(c1);

    add_heading(c1, "Checkboxes, Radios & Switches");
    // Row 1: unchecked checkbox + checked radio
    {
        auto row = add_row();
        {
            auto cb = std::make_unique<Checkbox>(Rect{}, "Option");
            cb->setAutoSize(AutoSize::Height);
            cb->setChecked(false);
            row->addChild(std::move(cb));
        }
        {
            auto rb = std::make_unique<RadioButton>(Rect{}, "Small");
            rb->setGroup("size");
            rb->setChecked(true);
            rb->setAutoSize(AutoSize::Height);
            row->addChild(std::move(rb));
        }
        c1->addChild(std::move(row));
    }
    // Row 2: checked checkbox + unchecked radio
    {
        auto row = add_row();
        {
            auto cb = std::make_unique<Checkbox>(Rect{}, "Enabled");
            cb->setChecked(true);
            cb->setAutoSize(AutoSize::Height);
            row->addChild(std::move(cb));
        }
        {
            auto rb = std::make_unique<RadioButton>(Rect{}, "Medium");
            rb->setGroup("size");
            rb->setAutoSize(AutoSize::Height);
            row->addChild(std::move(rb));
        }
        c1->addChild(std::move(row));
    }
    // Row 3: disabled checkbox + disabled radio
    {
        auto row = add_row();
        {
            auto cb = std::make_unique<Checkbox>(Rect{}, "Locked");
            cb->setEnabled(false);
            cb->setAutoSize(AutoSize::Height);
            row->addChild(std::move(cb));
        }
        {
            auto rb = std::make_unique<RadioButton>(Rect{}, "Disabled");
            rb->setGroup("size");
            rb->setEnabled(false);
            rb->setAutoSize(AutoSize::Height);
            row->addChild(std::move(rb));
        }
        c1->addChild(std::move(row));
    }
    // Row 4: with callback checkbox + unchecked radio
    {
        auto row = add_row();
        {
            auto cb = std::make_unique<Checkbox>(Rect{}, "Log toggle");
            cb->setAutoSize(AutoSize::Height);
            row->addChild(std::move(cb));
        }
        {
            auto rb = std::make_unique<RadioButton>(Rect{}, "Large");
            rb->setGroup("size");
            rb->setAutoSize(AutoSize::Height);
            row->addChild(std::move(rb));
        }
        c1->addChild(std::move(row));
    }
    // Switches: off, on, disabled
    {
        auto row = add_row();
        {
            auto sw = std::make_unique<Switch>(Rect{}, "Wi-Fi");
            sw->setAutoSize(AutoSize::Height);
            sw->setChecked(false);
            row->addChild(std::move(sw));
        }
        {
            auto sw = std::make_unique<Switch>(Rect{}, "Bluetooth");
            sw->setAutoSize(AutoSize::Height);
            sw->setChecked(true);
            row->addChild(std::move(sw));
        }
        c1->addChild(std::move(row));
    }
    {
        auto row = add_row();
        {
            auto sw = std::make_unique<Switch>(Rect{}, "Airplane");
            sw->setAutoSize(AutoSize::Height);
            sw->setEnabled(false);
            row->addChild(std::move(sw));
        }
        {
            auto sw = std::make_unique<Switch>(Rect{}, "VPN");
            sw->setAutoSize(AutoSize::Height);
            sw->setChecked(true);
            sw->setEnabled(false);
            row->addChild(std::move(sw));
        }
        c1->addChild(std::move(row));
    }

    // ────────────────────────────────────────────────────────────────────
    //  Column 2 — TextInputs & Dialogs & Shortcuts
    // ────────────────────────────────────────────────────────────────────
    auto* c2 = add_column();

    add_heading(c2, "Slider & Color");
    {
        auto sl = std::make_unique<Slider>(Rect{}, 0.5f);
        sl->setAutoSize(AutoSize::Height);
        sl->setShowValue(true);
        c2->addChild(std::move(sl));
    }
    // Color picker — lets you change the accent color dynamically
    {
        auto cp = std::make_unique<ColorPicker>(Rect{}, materialTheme.colors.accent);
        cp->setAutoSize(AutoSize::Height);
        cp->setOnChange([&win](Color c) {
            auto t = win.theme();
            t.colors.accent = c;
            t.colors.accent_pressed =
                Color{static_cast<std::uint8_t>(c.r * 0.8f),
                      static_cast<std::uint8_t>(c.g * 0.8f),
                      static_cast<std::uint8_t>(c.b * 0.8f),
                      c.a};
            win.setTheme(t);
        });
        c2->addChild(std::move(cp));
    }

    add_spacer(c2);

    add_heading(c2, "Text Inputs");
    // Plain (non-growable — fixed width from layout, type long text to see horizontal scrollbar)
    {
        auto ti = std::make_unique<TextInput>(Rect{}, "Type here...");
        ti->setGrowable(false);
        ti->setAutoSize(AutoSize::Height);
        c2->addChild(std::move(ti));
    }
    // With prefix icon
    {
        auto ti = std::make_unique<TextInput>(Rect{}, "Search...");
        ti->setPrefix($(Icons::search << " "));
        c2->addChild(std::move(ti));
    }
    // Multiline (fixed height — type many lines to see vertical scrollbar)
    {
        auto ti = std::make_unique<TextInput>(Rect{0, 0, 0, 80}, "Multi-line text...");
        ti->setMultiline(true);
        ti->setGrowable(false);
        ti->setAutoSize(AutoSize::Height);
        c2->addChild(std::move(ti));
    }
    // With prefix + events
    {
        auto ti = std::make_unique<TextInput>(Rect{}, "Email");
        ti->setPrefix($(Icons::at_sign << " "));
        ti->setOnChange([](const std::string&) {});
        c2->addChild(std::move(ti));
    }

    add_spacer(c2);

    add_heading(c2, "Dialogs");
    // Row 1: Plain OK + Yes / No
    {
        auto row = add_row();
        {
            auto btn = std::make_unique<Button>(Rect{}, $(Icons::badge_info << " Plain OK"));
            btn->setOnClick([&win]() {
                win.showDialog("Information",
                    "A plain info dialog with an OK button.",
                    Window::DialogButtons::Ok);
            });
            row->addChild(std::move(btn));
        }
        {
            auto btn = std::make_unique<Button>(Rect{}, $(Icons::alert_triangle << " Yes / No"));
            btn->setOnClick([&win]() {
                win.showDialog("Confirm", "Are you sure?",
                    Window::DialogButtons::YesNo);
            });
            row->addChild(std::move(btn));
        }
        c2->addChild(std::move(row));
    }
    // Row 2: Custom inline (full width)
    {
        auto btn = std::make_unique<Button>(Rect{}, $(Icons::cog << " Custom"));
        btn->setAutoSize(AutoSize::Height);
        btn->setOnClick([&win]() {
            win.showDialog("Custom Dialog",
                $(Icons::check << " Operation completed"),
                {$(Icons::check << " OK"),
                 $(Icons::x << " Cancel"),
                 $(Icons::circle_question_mark << " Help")});
        });
        c2->addChild(std::move(btn));
    }

    add_spacer(c2);

    add_heading(c2, "Shortcuts");
    {
        auto lbl = std::make_unique<Label>(Rect{},
            "F1=Light  F2=Dark  F3=Solarized  F4=Material  Esc=Quit",
            Label::Align::Left);
        c2->addChild(std::move(lbl));
    }

    win.setEventHandler([&win, &materialTheme](const Event& e, Window&) -> EventResult {
        if (auto* ke = std::get_if<KeyDownEvent>(&e)) {
            switch (ke->key) {
            case GLFW_KEY_F1:       win.setTheme(Theme::defaultLight());   return EventResult::Consumed;
            case GLFW_KEY_F2:       win.setTheme(Theme::defaultDark());    return EventResult::Consumed;
            case GLFW_KEY_F3:       win.setTheme(Theme::solarized());      return EventResult::Consumed;
            case GLFW_KEY_F4:       win.setTheme(materialTheme);       return EventResult::Consumed;
            case GLFW_KEY_ESCAPE:   win.requestClose();                    return EventResult::Consumed;
            default: break;
            }
        }
        return EventResult::Ignored;
    });

    // Quit
    {
        auto btn = std::make_unique<Button>(Rect{}, $(Icons::door_open << " Quit"));
        btn->setAutoSize(AutoSize::Height);
        btn->setOnClick([&win]() { win.requestClose(); });
        c2->addChild(std::move(btn));
    }

    win.run();
    return 0;
}
