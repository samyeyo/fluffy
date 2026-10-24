// examples/multiwindow/main.cpp
// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
// multiwindow — demonstrates Application::get() static singleton with two
// concurrent windows, per-window Theme and manual update pumping.
//
// MIT License - Copyright (c) 2026 - Samir Tine
// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

#include <fluffy/application.hpp>
#include <fluffy/button.hpp>
#include <fluffy/icons.hpp>
#include <fluffy/inline.hpp>
#include <fluffy/label.hpp>
#include <fluffy/theme.hpp>
#include <fluffy/window.hpp>

using namespace fluffy;

int main() {
    // ––––––––[ window A ] light theme — uses Window::open, auto-registers
    Window light = Window::open("Fluffy — Light", 440, 300);
    if (!light) return 1;
    light.setTheme(Theme::defaultLight());
    {
        auto& col = light.verticalLayout();
        col.setPadding(24.0f);
        col.setSpacing(12.0f);
        col.align(Align::Center);
        col.justify(Justify::Center);
        col.add<Label>($("Light window " << Icons::sun << " per-window Theme"))
            .setTextSize(18.0f)
            .setHorizontal(Label::Align::Center);
        col.add<Label>("Close this window — the other stays open", Label::Align::Center)
            .setTextSize(13.0f);
        col.add<Button>($(Icons::x << " Close Light"))
            .onClick([&] { light.requestClose(); });
        col.add<Button>($(Icons::log_out << " Quit App"))
            .onClick([] { Application::get().quit(); });
    }

    // ––––––––[ window B ] dark theme — uses Window::create, per-window Theme
    Window::Config cfg;
    cfg.title = "Fluffy — Dark";
    cfg.width = 440;
    cfg.height = 300;
    cfg.theme = Theme::defaultDark();
    Window dark = Window::create(cfg).unwrap();
    {
        auto& col = dark.verticalLayout();
        col.setPadding(24.0f);
        col.setSpacing(12.0f);
        col.align(Align::Center);
        col.justify(Justify::Center);
        col.add<Label>($("Dark window " << Icons::moon << " per-window Theme"))
            .setTextSize(18.0f)
            .setHorizontal(Label::Align::Center);
        col.add<Label>("This window has its own Theme", Label::Align::Center)
            .setTextSize(13.0f);
        col.add<Button>($(Icons::x << " Close Dark"))
            .onClick([&] { dark.requestClose(); });
    }

    // ––––––––[ run ] global loop pumps both windows; exits when all closed or quit()
    // Alternative manual pump:
    // while (Application::get().update(1.0f / 60.0f)) {}
    // or auto dt: while (Application::get().update()) {}
    Application::get().run();

    return 0;
}
