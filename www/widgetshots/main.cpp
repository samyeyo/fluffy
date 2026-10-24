// www/widgetshots/main.cpp
// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
// widgetshots — renders one documentation PNG per drawable widget into
// www/assets/widgets/, using hidden windows and the real drawing code.
// It is a developer tool; www/gendocs.py builds it on demand.
//
// MIT License - Copyright (c) 2026 - Samir Tine
// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

#include <fluffy/button.hpp>
#include <fluffy/checkbox.hpp>
#include <fluffy/color_picker.hpp>
#include <fluffy/combobox.hpp>
#include <fluffy/container.hpp>
#include <fluffy/icons.hpp>
#include <fluffy/image.hpp>
#include <fluffy/inline.hpp>
#include <fluffy/label.hpp>
#include <fluffy/layout.hpp>
#include <fluffy/listview.hpp>
#include <fluffy/menu.hpp>
#include <fluffy/nav.hpp>
#include <fluffy/progress_ring.hpp>
#include <fluffy/radiobutton.hpp>
#include <fluffy/slider.hpp>
#include <fluffy/switch.hpp>
#include <fluffy/table.hpp>
#include <fluffy/tabview.hpp>
#include <fluffy/textinput.hpp>
#include <fluffy/theme.hpp>
#include <fluffy/tooltip.hpp>
#include <fluffy/webview.hpp>
#include <fluffy/window.hpp>

#include <GLFW/glfw3.h>

#include <cstdio>
#include <filesystem>
#include <memory>
#include <string>
#include <utility>
#include <vector>

using namespace fluffy;

namespace {

// ––––––––[ capture ] renders a hidden window and saves it as a PNG
template <typename F>
bool capture(const char* name, int w, int h, F build) {
    Window::Config cfg;
    cfg.title = "Fluffy";
    cfg.width = w;
    cfg.height = h;
    cfg.resizable = false;
    cfg.vsync = false;
    cfg.visible = false;
    cfg.decorated = false;
    cfg.theme = Theme::defaultLight();
    auto res = Window::create(cfg);
    if (!res) {
        std::fprintf(stderr, "widgetshots: create failed for %s\n", name);
        return false;
    }
    Window win = std::move(res.value());
    build(win);
    win.renderFrame();
    win.renderFrame();
    const std::string out =
        std::string("www/assets/widgets/") + name + ".png";
    const bool ok = win.saveScreenshot(out);
    std::printf("widgetshots: %-16s -> %s\n", name, ok ? "ok" : "FAILED");
    return ok;
}

// ––––––––[ hover ] settles hover animation over a widget
void hover(Window& win, float x, float y) {
    win.setCursorPosition(x, y);
    for (int i = 0; i < 14; ++i) win.doHoverTick(0.05f);
}

// ––––––––[ item ] builds a menu entry
Menu::Item item(const SvgIconRef& icon, std::string label,
                std::string shortcut = {}) {
    Menu::Item it;
    it.icon = icon;
    it.label = std::move(label);
    it.shortcut = std::move(shortcut);
    return it;
}

// ––––––––[ navItem ] builds a navigation entry
Nav::NavItem navItem(const SvgIconRef& icon, std::string label,
                     std::string id) {
    Nav::NavItem it;
    it.icon = icon;
    it.label = std::move(label);
    it.id = std::move(id);
    return it;
}

} // namespace

int main() {
    std::filesystem::create_directories("www/assets/widgets");
    bool ok = true;

    // ––––––––[ window ] a complete window holding a small app layout
    ok &= capture("window", 640, 380, [](Window& win) {
        auto& title = win.add<Label>(Rect{32, 24, 380, 34}, "My App");
        title.setTextSize(24.0f);

        auto& nav = win.add<Nav>(Rect{32, 80, 150, 264});
        Nav::NavGroup workspace;
        workspace.label = "Workspace";
        workspace.id = "workspace";
        workspace.icon = Icons::folder;
        workspace.items.push_back(navItem(Icons::layers, "Home", "home"));
        workspace.items.push_back(
            navItem(Icons::folder_open, "Projects", "projects"));
        workspace.items.push_back(
            navItem(Icons::cog, "Settings", "settings"));
        nav.addGroup(std::move(workspace));
        nav.expandAll();
        for (int i = 0; i < 20; ++i) nav.tick(0.05f, win.theme());
        nav.setSelectedItem("home");

        auto& heading = win.add<Label>(Rect{210, 80, 398, 26},
            "Welcome back");
        heading.setTextSize(18.0f);
        auto& body = win.add<Label>(Rect{210, 116, 398, 60},
            "This window owns every widget on screen: a navigation panel, "
            "some controls and a row of actions.");
        body.setWordWrap(true);
        win.add<Checkbox>(Rect{210, 188, 320, 28},
            "Enable notifications").setChecked(true);
        win.add<Slider>(Rect{210, 228, 360, 32}, 0.65f);
        win.add<Button>(Rect{210, 282, 180, 40},
            $(Icons::check << " Save changes"));
        win.add<Button>(Rect{402, 282, 120, 40}, "Cancel");
    });

    // ––––––––[ label ] plain, rich and wrapped text
    ok &= capture("label", 560, 260, [](Window& win) {
        auto& title = win.add<Label>(
            Rect{40, 36, 480, 40}, "The quick brown fox");
        title.setTextSize(24.0f);
        win.add<Label>(Rect{40, 92, 480, 32},
            $(Icons::zap << " A label can mix text with an icon"));
        auto& wrapped = win.add<Label>(Rect{40, 140, 480, 70},
            "A label can wrap long text across several lines so it fits "
            "inside a narrow column without overflowing.");
        wrapped.setWordWrap(true);
        win.add<Label>(Rect{40, 218, 480, 24},
            "Right-aligned text", Label::Align::Right);
    });

    // ––––––––[ button ] plain, icon, disabled and hovered states
    ok &= capture("button", 640, 220, [](Window& win) {
        win.add<Button>(Rect{40, 90, 160, 40}, "Save changes");
        win.add<Button>(Rect{216, 90, 160, 40},
            $(Icons::download << " Download"));
        auto& off = win.add<Button>(Rect{392, 90, 120, 40}, "Disabled");
        off.setEnabled(false);
        win.add<Button>(Rect{528, 90, 80, 40}, "Hover");
        hover(win, 568.0f, 110.0f);
    });

    // ––––––––[ textinput ] placeholder, filled and focused
    ok &= capture("textinput", 560, 240, [](Window& win) {
        win.add<TextInput>(Rect{40, 44, 480, 40}, "Your name");
        auto& filled = win.add<TextInput>(Rect{40, 104, 480, 40});
        filled.setText("Ada Lovelace");
        win.requestFocus(&filled);
        win.add<TextInput>(Rect{40, 164, 480, 52}, "Multi-line notes")
            .setMultiline(true);
    });

    // ––––––––[ image ] a PNG scaled to fill its bounds
    ok &= capture("image", 560, 300, [](Window& win) {
        auto& img = win.add<Image>(Rect{40, 40, 480, 220},
            "www/fluffy-light.png");
        img.setScaleMode(ScaleMode::Fit);
        img.setBorder(true);
    });

    // ––––––––[ checkbox ] checked, unchecked and disabled
    ok &= capture("checkbox", 360, 220, [](Window& win) {
        win.add<Checkbox>(Rect{40, 50, 240, 28}, "Show grid")
            .setChecked(true);
        win.add<Checkbox>(Rect{40, 100, 240, 28}, "Snap to pixels");
        auto& off = win.add<Checkbox>(Rect{40, 150, 240, 28},
            "Locked option");
        off.setChecked(true);
        off.setEnabled(false);
    });

    // ––––––––[ radiobutton ] one selected option in a group
    ok &= capture("radiobutton", 320, 250, [](Window& win) {
        auto& small = win.add<RadioButton>(Rect{40, 46, 200, 28}, "Small");
        auto& medium = win.add<RadioButton>(Rect{40, 96, 200, 28}, "Medium");
        auto& large = win.add<RadioButton>(Rect{40, 146, 200, 28}, "Large");
        auto& huge = win.add<RadioButton>(Rect{40, 196, 200, 28}, "Huge");
        for (RadioButton* rb : {&small, &medium, &large, &huge})
            rb->setGroup("size");
        medium.setChecked(true);
    });

    // ––––––––[ switch ] on, off and disabled
    ok &= capture("switch", 360, 220, [](Window& win) {
        win.add<Switch>(Rect{40, 50, 240, 28}, "Wi-Fi")
            .setChecked(true);
        win.add<Switch>(Rect{40, 100, 240, 28}, "Bluetooth");
        auto& off = win.add<Switch>(Rect{40, 150, 240, 28},
            "Airplane mode");
        off.setChecked(true);
        off.setEnabled(false);
    });

    // ––––––––[ slider ] three values, one disabled
    ok &= capture("slider", 560, 250, [](Window& win) {
        win.add<Slider>(Rect{40, 46, 480, 32}, 0.25f);
        win.add<Slider>(Rect{40, 106, 480, 32}, 0.7f);
        auto& off = win.add<Slider>(Rect{40, 166, 480, 32}, 0.5f);
        off.setEnabled(false);
    });

    // ––––––––[ color_picker ] editable HSV picker
    ok &= capture("color_picker", 560, 300, [](Window& win) {
        auto& picker = win.add<ColorPicker>(Rect{40, 40, 480, 220},
            Color::fromHex(0xFF6A46E5));
        picker.setFocused(true);
    });

    // ––––––––[ combobox ] closed selection and an open dropdown
    ok &= capture("combobox", 400, 320, [](Window& win) {
        auto& size = win.add<ComboBox>(Rect{40, 44, 220, 40});
        size.setItems({"Small", "Medium", "Large", "Extra large"});
        size.setSelectedIndex(1);
        auto& open = win.add<ComboBox>(Rect{40, 110, 220, 40});
        open.setItems({"Small", "Medium", "Large", "Extra large"});
        open.setSelectedIndex(1);
        open.openPopup(win);
    });

    // ––––––––[ listview ] a scrolling list with one selected row
    ok &= capture("listview", 400, 300, [](Window& win) {
        auto& list = win.add<ListView>(Rect{40, 40, 320, 220});
        list.setItems({"Inbox", "Starred", "Sent", "Drafts", "Archive",
            "Spam", "Trash"});
        list.setSelected(2);
    });

    // ––––––––[ tabview ] three tabs with content in the active one
    ok &= capture("tabview", 560, 320, [](Window& win) {
        auto& tabs = win.add<TabView>(Rect{40, 40, 480, 240});
        tabs.addTab("General", "general");
        tabs.addTab("Advanced", "advanced");
        tabs.addTab("About", "about");
        tabs.setSelected(0);
        auto* content = tabs.content();
        content->add<Label>("Start Fluffy when I sign in");
        content->add<Checkbox>(Rect{}, "Check for updates")
            .setChecked(true);
        content->add<Button>(Rect{}, "Manage accounts");
    });

    // ––––––––[ table ] headers, rows and a selected cell
    ok &= capture("table", 640, 320, [](Window& win) {
        auto& table = win.add<Table>(Rect{40, 40, 560, 240});
        std::vector<Table::Column> cols;
        Table::Column name;
        name.label = "Name";
        name.width = 200.0f;
        cols.push_back(name);
        Table::Column role;
        role.label = "Role";
        role.width = 160.0f;
        cols.push_back(role);
        Table::Column active;
        active.label = "Active";
        active.width = 90.0f;
        active.kind = Table::Column::Kind::Bool;
        cols.push_back(active);
        table.setColumns(std::move(cols));
        table.setRows({
            {"Ada Lovelace", "Engineer", "true"},
            {"Grace Hopper", "Admiral", "false"},
            {"Alan Turing", "Researcher", "true"},
            {"Katherine Johnson", "Analyst", "true"},
        });
        table.setSelected(1, 0);
    });

    // ––––––––[ progress_ring ] determinate and indeterminate rings
    ok &= capture("progress_ring", 480, 240, [](Window& win) {
        win.add<ProgressRing>(Rect{50, 70, 90, 90}, 0.25f);
        win.add<ProgressRing>(Rect{190, 70, 90, 90}, 0.7f);
        auto& busy = win.add<ProgressRing>(Rect{330, 70, 90, 90}, -1.0f);
        busy.setIndeterminate(true);
        for (int i = 0; i < 10; ++i) busy.tick(0.05f, win.theme());
    });

    // ––––––––[ menu ] horizontal bar and a vertical dropdown
    ok &= capture("menu", 560, 330, [](Window& win) {
        auto& bar = win.add<Menu>(Rect{40, 32, 480, 36},
            Menu::Orientation::Horizontal);
        bar.addItem(item(Icons::folder, "File"));
        bar.addItem(item(Icons::cog, "Edit"));
        bar.addItem(item(Icons::search, "View"));
        bar.addItem(item(Icons::bell, "Help"));

        auto& list = win.add<Menu>(Rect{40, 84, 260, 220},
            Menu::Orientation::Vertical);
        list.addItem(item(Icons::door_open, "Open", "Ctrl+O"));
        list.addItem(item(Icons::folder_open, "Open folder"));
        Menu::Item sep;
        sep.separator = true;
        list.addItem(std::move(sep));
        list.addItem(item(Icons::download, "Export", "Ctrl+E"));
        Menu::Item locked = item(Icons::trash, "Delete");
        locked.enabled = false;
        list.addItem(std::move(locked));
    });

    // ––––––––[ nav ] expanded groups and a selected item
    ok &= capture("nav", 360, 380, [](Window& win) {
        auto& nav = win.add<Nav>(Rect{40, 30, 280, 320});
        Nav::NavGroup workspace;
        workspace.label = "Workspace";
        workspace.id = "workspace";
        workspace.icon = Icons::folder;
        workspace.items.push_back(navItem(Icons::layers, "Home", "home"));
        workspace.items.push_back(
            navItem(Icons::folder, "Projects", "projects"));
        workspace.items.push_back(
            navItem(Icons::cog, "Settings", "settings"));
        nav.addGroup(std::move(workspace));

        Nav::NavGroup account;
        account.label = "Account";
        account.id = "account";
        account.icon = Icons::user;
        account.items.push_back(
            navItem(Icons::user, "Profile", "profile"));
        account.items.push_back(
            navItem(Icons::bell, "Notifications", "notifications"));
        nav.addGroup(std::move(account));
        nav.expandAll();
        for (int i = 0; i < 20; ++i) nav.tick(0.05f, win.theme());
        nav.setSelectedItem("home");
    });

    // ––––––––[ tooltip ] hover hint attached to a button
    ok &= capture("tooltip", 420, 220, [](Window& win) {
        auto& btn = win.add<Button>(Rect{40, 60, 200, 40},
            $(Icons::trash << " Delete"));
        win.add<Label>(Rect{40, 150, 480, 24},
            "Hover the button to see its tooltip.");

        Tooltip::showFor(&btn, "Deletes the selected item", win);
        if (win.popup()) {
            for (int i = 0; i < 10; ++i)
                win.popup()->tick(0.05f, win.theme());
        }
    });

    // ––––––––[ dialog ] a confirmation dialog's content
    ok &= capture("dialog", 420, 210, [](Window& win) {
        auto& col = win.verticalLayout();
        col.setPadding(20.0f);
        col.setSpacing(10.0f);
        auto& title = col.add<Label>("Delete this file?",
            Label::Align::Center);
        title.setTextSize(18.0f);
        col.add<Label>("This action cannot be undone.",
            Label::Align::Center);
        auto& row = col.add<Container>();
        row.setHorizontal(8.0f);
        row.justify(Justify::Center);
        row.add<Button>("Cancel");
        row.add<Button>("Delete");
    });

    return ok ? 0 : 1;
}
