// examples/helloworld/main.cpp
// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
// helloworld — minimal Fluffy demonstration with labels, buttons and icons.
//
// MIT License - Copyright (c) 2026 - Samir Tine
// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

#include <fluffy/fluffy.hpp>

using namespace fluffy;

int main() {
    Window window = Window::open("Hello Fluffy", 400, 240);
    if (!window) {
        return 1;
    }

    auto& layout = window.verticalLayout();
    layout.setPadding(32.0f).setSpacing(8.0f);
    layout.align(Align::Center);
    layout.justify(Justify::SpaceBetween);

    layout.add<Label>($("Hello from Fluffy " << Color::fromHex(0xFFFF0000) << Icons::heart))
        .setTextSize(28.0f)
        .setHorizontal(Label::Align::Center);

    layout.add<Button>($(Icons::door_open << " Quit"))
        .onClick([&] { window.close(); });

    window.run();
}
