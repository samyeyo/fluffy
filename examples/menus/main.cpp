// examples/menus/main.cpp

// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

// Menus — demonstrates horizontal and vertical Menu widgets.

#include <string>

#include <fluffy/button.hpp>
#include <fluffy/container.hpp>
#include <fluffy/error.hpp>
#include <fluffy/geom.hpp>
#include <fluffy/icons.hpp>
#include <fluffy/inline.hpp>
#include <fluffy/label.hpp>
#include <fluffy/layout.hpp>
#include <fluffy/menu.hpp>
#include <fluffy/theme.hpp>
#include <fluffy/widget.hpp>
#include <fluffy/window.hpp>

int main() {
    using namespace fluffy;

    Window::Config cfg;
    cfg.title     = "Fluffy — Menus";
    cfg.width     = 600;
    cfg.height    = 480;
    cfg.min_width  = 400;
    cfg.min_height = 320;
    cfg.theme     = Theme::defaultLight();
    auto win_or_err = Window::create(cfg);
    if (!win_or_err) {
        return 1;
    }
    Window& win = win_or_err.value();

    // ── Root: vertical layout ──
    auto& root = win.add<Container>(Rect(10.0f, 10.0f, 580.0f, 460.0f));
    {
        auto vl = std::make_unique<VerticalLayout>();
        vl->setSpacing(12.0f);
        root.setLayout(std::move(vl));
    }

    // ── Horizontal menu bar (top) ──
    {
        auto row = std::make_unique<Container>(Rect{0, 0, 580, 0});
        auto hl = std::make_unique<HorizontalLayout>();
        hl->setSpacing(0.0f);
        row->setLayout(std::move(hl));
        row->setAutoSize(AutoSize::Height);

        // Build a submenu for File
        auto file_sub = std::make_unique<Menu>(Rect{}, Menu::Orientation::Vertical);
        {
            Menu::Item i;
            i.icon = Icons::file_text; i.label = "New"; i.shortcut = "Ctrl+N";
            file_sub->addItem(std::move(i));
        }
        {
            Menu::Item i;
            i.icon = Icons::folder_open; i.label = "Open..."; i.shortcut = "Ctrl+O";
            file_sub->addItem(std::move(i));
        }
        { Menu::Item i; i.separator = true; file_sub->addItem(std::move(i)); }
        {
            Menu::Item i;
            i.icon = Icons::save; i.label = "Save"; i.shortcut = "Ctrl+S";
            file_sub->addItem(std::move(i));
        }
        {
            Menu::Item i;
            i.icon = Icons::edit; i.label = "Save As..."; i.shortcut = "Ctrl+Shift+S";
            file_sub->addItem(std::move(i));
        }
        { Menu::Item i; i.separator = true; file_sub->addItem(std::move(i)); }
        {
            // Reopen... with submenu
            Menu::Item i;
            i.icon = Icons::refresh;
            i.label = "Reopen...";
            auto recent = std::make_unique<Menu>(Rect{}, Menu::Orientation::Vertical);
            for (const char* name : {"Recent Project A", "Recent Project B", "Recent Project C"}) {
                Menu::Item ri;
                ri.icon = Icons::file_text;
                ri.label = name;
                recent->addItem(std::move(ri));
            }
            i.submenu = std::move(recent);
            file_sub->addItem(std::move(i));
        }

        // Build a submenu for Edit
        auto edit_sub = std::make_unique<Menu>(Rect{}, Menu::Orientation::Vertical);
        {
            Menu::Item i;
            i.icon = Icons::copy; i.label = "Copy"; i.shortcut = "Ctrl+C";
            edit_sub->addItem(std::move(i));
        }
        {
            Menu::Item i;
            i.icon = Icons::clipboard_paste; i.label = "Paste"; i.shortcut = "Ctrl+V";
            edit_sub->addItem(std::move(i));
        }
        {
            Menu::Item i;
            i.icon = Icons::star; i.label = "Cut"; i.shortcut = "Ctrl+X";
            edit_sub->addItem(std::move(i));
        }
        { Menu::Item i; i.separator = true; edit_sub->addItem(std::move(i)); }
        {
            Menu::Item i;
            i.icon = Icons::search; i.label = "Find..."; i.shortcut = "Ctrl+F";
            edit_sub->addItem(std::move(i));
        }

        // Build a submenu for View
        auto view_sub = std::make_unique<Menu>(Rect{}, Menu::Orientation::Vertical);
        {
            Menu::Item i;
            i.icon = Icons::zoom_in; i.label = "Zoom In"; i.shortcut = "Ctrl+=";
            view_sub->addItem(std::move(i));
        }
        {
            Menu::Item i;
            i.icon = Icons::zoom_out; i.label = "Zoom Out"; i.shortcut = "Ctrl+-";
            view_sub->addItem(std::move(i));
        }

        // Build a submenu for Help
        auto help_sub = std::make_unique<Menu>(Rect{}, Menu::Orientation::Vertical);
        {
            Menu::Item i;
            i.icon = Icons::book_open; i.label = "Documentation";
            help_sub->addItem(std::move(i));
        }
        {
            Menu::Item i;
            i.label = "About";
            help_sub->addItem(std::move(i));
        }

        for (const auto& def : {
            std::pair{"File", file_sub.get()},
            {"Edit", edit_sub.get()},
            {"View", view_sub.get()},
            {"Help", help_sub.get()},
        }) {
            auto m = std::make_unique<Menu>(Rect{}, Menu::Orientation::Horizontal);
            Menu::Item item;
            item.label = def.first;
            item.submenu = std::make_unique<Menu>(Rect{}, Menu::Orientation::Vertical);
            // Copy submenu items
            for (const auto& si : def.second->items()) {
                Menu::Item copy;
                copy.icon = si.icon;
                copy.label = si.label;
                copy.shortcut = si.shortcut;
                copy.on_click = si.on_click;
                copy.enabled = si.enabled;
                copy.separator = si.separator;
                item.submenu->addItem(std::move(copy));
            }
            m->addItem(std::move(item));
            row->addChild(std::move(m));
        }
        root.addChild(std::move(row));
    }

    // ── Instruction label ──
    {
        auto lbl = std::make_unique<Label>(Rect{},
            "Click a menu bar item above, or try the vertical menu below.",
            Label::Align::Left);
        root.addChild(std::move(lbl));
    }

    // ── Vertical menu (full-width) ──
    {
        auto menu = std::make_unique<Menu>(Rect{}, Menu::Orientation::Vertical);
        {
            Menu::Item item;
            item.icon    = Icons::file_text;
            item.label   = "New File";
            item.shortcut = "Ctrl+N";
            menu->addItem(std::move(item));
        }
        {
            Menu::Item item;
            item.icon    = Icons::folder_open;
            item.label   = "Open...";
            item.shortcut = "Ctrl+O";
            menu->addItem(std::move(item));
        }
        {
            Menu::Item item;
            item.icon    = Icons::save;
            item.label   = "Save";
            item.shortcut = "Ctrl+S";
            menu->addItem(std::move(item));
        }
        {
            Menu::Item item;
            item.separator = true;
            menu->addItem(std::move(item));
        }
        {
            Menu::Item item;
            item.icon    = Icons::edit;
            item.label   = "Save As...";
            item.shortcut = "Ctrl+Shift+S";
            menu->addItem(std::move(item));
        }
        {
            Menu::Item item;
            item.icon    = Icons::trash;
            item.label   = "Delete";
            item.shortcut = "Del";
            item.enabled = false;
            menu->addItem(std::move(item));
        }
        menu->setAutoSize(AutoSize::Height);
        root.addChild(std::move(menu));
    }

    // ── Second instruction / separator label ──
    {
        auto lbl = std::make_unique<Label>(Rect{},
            "Vertical menu items log to the console when clicked.",
            Label::Align::Left);
        root.addChild(std::move(lbl));
    }

    // ── Button dropdown row ──
    {
        auto row = std::make_unique<Container>(Rect{0, 0, 580, 0});
        row->setAutoSize(AutoSize::Height);
        auto hl = std::make_unique<HorizontalLayout>();
        hl->setSpacing(12.0f);
        row->setLayout(std::move(hl));

        // Dropdown button
        {
            auto dd_menu = std::make_unique<Menu>(Rect{}, Menu::Orientation::Vertical);
            {
                Menu::Item i;
                i.icon = Icons::file_text; i.label = "New Document";
                dd_menu->addItem(std::move(i));
            }
            {
                Menu::Item i;
                i.icon = Icons::folder_open; i.label = "Open File...";
                dd_menu->addItem(std::move(i));
            }
            { Menu::Item i; i.separator = true; dd_menu->addItem(std::move(i)); }
            {
                Menu::Item i;
                i.icon = Icons::save; i.label = "Export";
                auto export_sub = std::make_unique<Menu>(Rect{}, Menu::Orientation::Vertical);
                {
                    Menu::Item si; si.label = "Export as PDF";
                    export_sub->addItem(std::move(si));
                }
                {
                    Menu::Item si; si.label = "Export as PNG";
                    export_sub->addItem(std::move(si));
                }
                i.submenu = std::move(export_sub);
                dd_menu->addItem(std::move(i));
            }

            auto btn = std::make_unique<Button>(Rect{}, $(Icons::cog << " Actions"));
            btn->setDropdownMenu(std::move(dd_menu));
            btn->setAutoSize(AutoSize::Height);
            row->addChild(std::move(btn));
        }

        // Quit button
        {
            auto btn = std::make_unique<Button>(Rect{}, $(Icons::door_open << " Quit"));
            btn->setAutoSize(AutoSize::Height);
            btn->setOnClick([&win]() { win.requestClose(); });
            row->addChild(std::move(btn));
        }

        root.addChild(std::move(row));
    }

    win.run();
    return 0;
}
