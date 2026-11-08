// examples/yamlui/main.cpp

// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

// yamlui — YAML-driven UI demo with EventDispatcher and showDialog.

#include <fluffy/button.hpp>
#include <fluffy/container.hpp>
#include <fluffy/textinput.hpp>
#include <fluffy/dispatcher.hpp>
#include <fluffy/event_arg.hpp>
#include <fluffy/geom.hpp>
#include <fluffy/icons.hpp>
#include <fluffy/label.hpp>
#include <fluffy/layout.hpp>
#include <fluffy/theme.hpp>
#include <fluffy/widget.hpp>
#include <fluffy/window.hpp>
#include <filesystem>
#include <string>
namespace {
std::string resolve_ui_yaml(const std::string& argv0) {
    namespace fs = std::filesystem;
    std::error_code ec;
            fs::path exe_path = fs::path{argv0};
            if (!exe_path.empty()) {
                exe_path = fs::absolute(exe_path, ec);
                if (ec) exe_path.clear();
                if (!exe_path.empty()) {
                    const fs::path exe_dir = exe_path.parent_path();
                    const fs::path alongside = exe_dir / "ui.yaml";
                    if (fs::is_regular_file(alongside, ec)) {
                        return fs::canonical(alongside, ec).string();
                    }
                }
            }
            if (fs::is_regular_file("ui.yaml", ec)) {
                return std::string{"ui.yaml"};
            }
            return {};
}
}  
int main(int argc, char** argv) {
    using namespace fluffy;
    Window::Config cfg;
    cfg.title  = "Fluffy - YAML UI Demo";
    cfg.width  = 800;
    cfg.height = 600;
    cfg.theme  = Theme::defaultLight();
    auto win_or_err = Window::create(cfg);
    if (!win_or_err) {
        return 1;
    }
    Window& win = win_or_err.value();
    win.dispatcher().registerHandler("handleSubmit",
        [&win](const EventArgument& arg) {
            std::string submitted_text;
            if (!arg.args.empty()) {
                submitted_text = arg.args[0];
            }
            if (!submitted_text.empty()) {
                win.showDialog("Submitted",
                    $(Icons::check << " You entered: " << submitted_text),
                    std::vector<Inline>{ $("OK") });
            }
        });
    win.dispatcher().registerHandler("submitForm",
        [](const EventArgument& arg) {
            [[maybe_unused]] Rect b = arg.widget ? arg.widget->bounds() : Rect{};
        });
    win.dispatcher().registerHandler("showModal",
        [&win](const EventArgument&) {
            win.showDialog(
                "Hello from Fluffy",
                "This is a true desktop dialog window powered by GLFW.",
                {
                    $(Icons::check << " Yes"),
                    $(Icons::x << " No"),
                    $(Icons::ban << " Cancel")
                });

        });
    const std::string yaml_path = resolve_ui_yaml(argc > 0 ? argv[0] : "");
    if (yaml_path.empty()) {
    } else {
        auto pres = win.load(yaml_path);
    }
    // Add a pencil icon prefix to the TextInput (child 1 of the root Container).
    // static_cast is safe here because the example controls the YAML structure.
    if (auto* root = static_cast<Container*>(win.rootWidget())) {
        if (auto* input = static_cast<TextInput*>(root->child(1))) {
            input->setPrefix($(Icons::pencil << " "));
        }
    }
    // Theme switcher row with a label showing the current theme
    auto& theme_row = win.add<Container>(Rect(0.0f, 356.0f, 800.0f, 44.0f));
    theme_row.setHorizontal(8.0f, 4.0f);
    auto theme_lbl = std::make_unique<Label>(Rect{}, "light", Label::Align::Left);
    theme_lbl->setTextSize(12.0f);
    Label* theme_lbl_ptr = theme_lbl.get();
    theme_row.addChild(std::move(theme_lbl));
    auto add_theme_btn = [&](Inline content, Theme theme, std::string name) {
        auto btn = std::make_unique<Button>(Rect{}, content);
        btn->setOnClick([&win, theme = std::move(theme), theme_lbl_ptr, n = std::move(name)] {
            win.setTheme(theme);
            theme_lbl_ptr->setText(n);
        });
        theme_row.addChild(std::move(btn));
    };
    add_theme_btn($(Icons::sun << " Light"),  Theme::defaultLight(),  "light");
    add_theme_btn($(Icons::moon << " Dark"),  Theme::defaultDark(),   "dark");
    add_theme_btn($(Icons::brush << " Solarized"), Theme::solarized(), "solarized");
    win.run();
    return 0;
}

