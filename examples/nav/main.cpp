// examples/nav/main.cpp

// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
// Nav — widget showcase demo listing all available widgets in the Nav panel.
//       Selecting a widget displays a live example in the right pane.
//
// MIT License - Copyright (c) 2026 - Samir Tine
// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

#include <string>
#include <vector>
#include <memory>
#include <set>
#include <filesystem>
#include <atomic>
#include <mutex>
#include <thread>
#if defined(FLUFFY_EMSCRIPTEN) || defined(__EMSCRIPTEN__)
#include <emscripten.h>
#endif

namespace fs = std::filesystem;

#include <fluffy/button.hpp>
#include <fluffy/checkbox.hpp>
#include <fluffy/color_picker.hpp>
#include <fluffy/constraint.hpp>
#include <fluffy/container.hpp>
#include <fluffy/error.hpp>
#include <fluffy/font.hpp>
#include <fluffy/geom.hpp>
#include <fluffy/icons.hpp>
#include <fluffy/combobox.hpp>
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
#include <fluffy/widget.hpp>
#include <fluffy/window.hpp>
#ifdef FLUFFY_ENABLE_WEBVIEW
#include <fluffy/webview.hpp>
#endif

#include <GLFW/glfw3.h>



int main() {
    using namespace fluffy;

    Window::Config cfg;
    cfg.title     = "Fluffy — Widget Showcase";
    cfg.width     = 920;
    cfg.height    = 640;
    cfg.min_width  = 680;
    cfg.min_height = 480;
    cfg.theme     = Theme::defaultLight();
    auto win_or_err = Window::create(cfg);
    if (!win_or_err) {
        return 1;
    }
    Window& win = win_or_err.value();

    // Material Dark theme
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

    // ── Root container (no layout — manual positioning for resize support) ──
    auto& root = win.add<Container>(Rect(0.0f, 0.0f, 920.0f, 640.0f));

    // ── Nav widget (left pane — widget browser) ──
    auto nav = std::make_unique<Nav>(Rect{0, 0, 200, 640});

    // Helper: build a NavItem
    auto make_item = [](const SvgIconRef& icon, const std::string& label,
                         const std::string& id, auto&& click) -> Nav::NavItem {
        Nav::NavItem item;
        item.icon = icon;
        item.label = label;
        item.id    = id;
        item.on_click = std::forward<decltype(click)>(click);
        return item;
    };

    // ── Group: Display ──
    {
        Nav::NavGroup g;
        g.label = "Display";
        g.id    = "grp_display";
        g.icon  = Icons::monitor;
        g.items.push_back(make_item(Icons::bookmark, "Label",   "page_label",   [] {}));
        g.items.push_back(make_item(Icons::crosshair, "Button", "page_button", [] {}));
        g.items.push_back(make_item(Icons::menu, "Nav",     "page_nav",     [] {}));
        g.items.push_back(make_item(Icons::flag, "Menu", "page_menu", [] {}));
        g.items.push_back(make_item(Icons::arrow_down, "Scroll", "page_scroll", [] {}));
        g.items.push_back(make_item(Icons::image, "Image", "page_image", [] {}));
#ifdef FLUFFY_ENABLE_WEBVIEW
        g.items.push_back(make_item(Icons::globe, "WebView", "page_webview", [] {}));
#endif
        g.items.push_back(make_item(Icons::a_large_small, "Font", "page_font", [] {}));
        nav->addGroup(std::move(g));
    }

    // ── Group: Layout ──
    {
        Nav::NavGroup g;
        g.label = "Layout";
        g.id    = "grp_layout";
        g.icon  = Icons::expand;
        g.items.push_back(make_item(Icons::anchor, "Constraints", "page_constraints", [] {}));
        nav->addGroup(std::move(g));
    }

    // ── Group: Input ──
    {
        Nav::NavGroup g;
        g.label = "Input";
        g.id    = "grp_input";
        g.icon  = Icons::pencil;
        g.items.push_back(make_item(Icons::pencil,    "Text Input", "page_textinput", [] {}));
        g.items.push_back(make_item(Icons::chart_bar, "Slider", "page_slider", [] {}));
        g.items.push_back(make_item(Icons::droplet,   "Color Picker", "page_colorpicker", [] {}));
        nav->addGroup(std::move(g));
    }

    // ── Group: Toggle ──
    {
        Nav::NavGroup g;
        g.label = "Toggle";
        g.id    = "grp_toggle";
        g.icon  = Icons::zap;
        g.items.push_back(make_item(Icons::badge_check, "Checkbox",     "page_checkbox", [] {}));
        g.items.push_back(make_item(Icons::circle_dot,  "Radio Button", "page_radio",    [] {}));
        g.items.push_back(make_item(Icons::power,       "Switch",       "page_switch",   [] {}));
        nav->addGroup(std::move(g));
    }

    // ── Group: Data ──
    {
        Nav::NavGroup g;
        g.label = "Data";
        g.id    = "grp_data";
        g.icon  = Icons::database;
        g.items.push_back(make_item(Icons::chevron_down, "ComboBox",     "page_combobox",  [] {}));
        g.items.push_back(make_item(Icons::clipboard_list,"ListView",     "page_listview",  [] {}));
        g.items.push_back(make_item(Icons::layers,       "TabView",      "page_tabview",   [] {}));
        g.items.push_back(make_item(Icons::blocks,       "Grid",         "page_grid",      [] {}));
        g.items.push_back(make_item(Icons::loader,       "ProgressRing", "page_progress", [] {}));
        g.items.push_back(make_item(Icons::badge_info,   "Tooltip",      "page_tooltip",   [] {}));
        g.items.push_back(make_item(Icons::database,     "Table",        "page_table",     [] {}));
        nav->addGroup(std::move(g));
    }

    // ── Group: Dialog ── (on WASM these use the browser alert()/confirm() fallback)
    {
        Nav::NavGroup g;
        g.label = "Dialogs";
        g.id    = "grp_dialog";
        g.icon  = Icons::inbox;
        g.items.push_back(make_item(Icons::badge_info,       "Info Dialog",    "page_info",    [] {}));
        g.items.push_back(make_item(Icons::badge_alert,      "Confirm Dialog", "page_confirm", [] {}));
        g.items.push_back(make_item(Icons::cog,              "Custom Dialog",  "page_custom",  [] {}));
        nav->addGroup(std::move(g));
    }

    // ── Group: Settings ──
    {
        Nav::NavGroup g;
        g.label = "Settings";
        g.id    = "grp_settings";
        g.icon  = Icons::cog;
        g.items.push_back(make_item(Icons::brush, "Theme Manager", "page_themes", [] {}));
        nav->addGroup(std::move(g));
    }

    // ── Page builders ──────────────────────────────────────────────────────
    // Each builds a Container* with example widgets. Called on Nav selection.
    // Page layout: header (auto-height) + examples (stretch-weight = 1, fills remaining space)

    // ── Page helper: initializes a page with header + examples container (fills remaining space) ──
    auto page_begin = [](Container* c, const std::string& title, const std::string& desc) -> Container* {
        auto vl = std::make_unique<VerticalLayout>();
        vl->setPadding(12.0f);
        vl->setSpacing(4.0f);
        c->setLayout(std::move(vl));

        auto heading = std::make_unique<Label>(Rect{}, title, Label::Align::Left);
        heading->setTextSize(18.0f);
        heading->setAutoSize(AutoSize::Height);
        c->addChild(std::move(heading));

        auto desc_lbl = std::make_unique<Label>(Rect{}, desc, Label::Align::Left);
        c->addChild(std::move(desc_lbl));

        auto spacer = std::make_unique<Label>(Rect{}, "");
        spacer->setTextSize(4.0f);
        spacer->setAutoSize(AutoSize::Height);
        c->addChild(std::move(spacer));

        auto ex = std::make_unique<Container>(Rect{});
        ex->setStretchWeight(1.0f);
        ex->setAutoSize(AutoSize::Width);
        auto ex_vl = std::make_unique<VerticalLayout>();
        ex_vl->setSpacing(8.0f);
        ex->setLayout(std::move(ex_vl));
        auto* ex_ptr = ex.get();
        c->addChild(std::move(ex));
        return ex_ptr;
    };

    // Convenience: add a horizontal row of widgets to a container
    auto add_row = [](Container* parent) -> Container* {
        auto row = std::make_unique<Container>(Rect{});
        auto hl = std::make_unique<HorizontalLayout>();
        hl->setSpacing(6.0f);
        row->setLayout(std::move(hl));
        row->setAutoSize(AutoSize::Height);
        auto* r = row.get();
        parent->addChild(std::move(row));
        return r;
    };

    // ── Background font scanning ──
    std::mutex font_mutex;
    std::atomic<bool> fonts_ready{false};
    std::vector<std::string> latin_fonts;
#if defined(FLUFFY_EMSCRIPTEN) || defined(__EMSCRIPTEN__)
    // Emscripten: no pthreads — run synchronously
    {
        auto result = listLatinFonts();
        std::sort(result.begin(), result.end(), [](const std::string& a, const std::string& b) {
            return fs::path(a).stem().string() < fs::path(b).stem().string();
        });
        std::lock_guard<std::mutex> lock(font_mutex);
        latin_fonts = std::move(result);
        fonts_ready = true;
    }
#else
    std::thread font_thread([&]() {
        auto result = listLatinFonts();
        std::sort(result.begin(), result.end(), [](const std::string& a, const std::string& b) {
            return fs::path(a).stem().string() < fs::path(b).stem().string();
        });
        fprintf(stderr, "[font scan] %zu Latin fonts found:\n", result.size());
        for (size_t i = 0; i < result.size(); ++i) {
            fprintf(stderr, "  [%zu] %s\n", i, fs::path(result[i]).stem().string().c_str());
        }
        {
            std::lock_guard<std::mutex> lock(font_mutex);
            latin_fonts = std::move(result);
            fonts_ready = true;
        }
    });
    font_thread.detach();
#endif

    // ── Page: Label ──
    auto build_label_page = [&](Container* c) {
        auto* ex = page_begin(c, "Label",
            "Labels display static text and icons. Supports inline formatting with colors, sizes, and styled icons.");
        if (!ex) return;

        auto add_lbl = [&](const std::string& text) {
            auto lbl = std::make_unique<Label>(Rect{}, text, Label::Align::Left);
            ex->addChild(std::move(lbl));
        };
        auto add_lbl_inline = [&](const Inline& inl) {
            auto lbl = std::make_unique<Label>(Rect{}, inl, Label::Align::Left);
            ex->addChild(std::move(lbl));
        };

        add_lbl("Plain short text");
        add_lbl("Text with medium length that wraps naturally");

        // Inline with icon + text
        add_lbl_inline($(Icons::check << " Done — inline icon with text"));

        // Colored inline content
        add_lbl_inline(
            Inline{} << Color{220, 50, 50, 255} << Icons::flame << Color{220, 50, 50, 255}
                     << " Colored text "
                     << Color{50, 120, 220, 255}
                     << Icons::droplet << Color{50, 120, 220, 255}
                     << " Styled icons");

        // Green success icon
        add_lbl_inline(
            Inline{} << Color{60, 180, 75, 255} << Icons::check_circle << Color{60, 180, 75, 255}
                     << " Success "
                     << Color{200, 160, 40, 255} << Icons::alert_triangle << Color{200, 160, 40, 255}
                     << " Warning");

        // Larger icon with text (color must come before the icon run)
        add_lbl_inline(
            Inline{} << Style{{}, 18.0f} << Color{180, 100, 200, 255} << Icons::bell
                     << Color{180, 100, 200, 255} << " Larger styled text");

        // Faux bold and italic via Style
        add_lbl_inline(
            Inline{} << Style{Color{50, 50, 180, 255}, 0.0f, true, false}
                     << Icons::bold << " Bold text "
                     << Style{Color{50, 180, 50, 255}, 0.0f, false, true}
                     << "\u00A0 Italic text");

        add_lbl_inline(
            Inline{} << Style{Color{200, 80, 80, 255}, 16.0f, true, false}
                     << "Bold heading style "
                     << Style{Color{100, 100, 100, 255}, 0.0f, false, false}
                     << "normal text");

        add_lbl_inline($(Icons::mail << "  " << Icons::smartphone << "  " << Icons::globe << " Mixed icons"));
    };

    // ── Page: Button ──
    auto build_button_page = [&](Container* c) {
        auto* ex = page_begin(c, "Button",
            "Buttons trigger actions. Supports text, inline content with icons, disabled state, and dropdown menus.");
        if (!ex) return;

        {
            auto* row = add_row(ex);
            {
                auto btn = std::make_unique<Button>(Rect{}, "Click Me");
                row->addChild(std::move(btn));
            }
            {
                auto btn = std::make_unique<Button>(Rect{}, $(Icons::send << " Send"));
                row->addChild(std::move(btn));
            }
            {
                auto btn = std::make_unique<Button>(Rect{}, Inline{} << Icons::cog);
                row->addChild(std::move(btn));
            }
        }
        {
            auto* row = add_row(ex);
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
        }
        {
            auto* row = add_row(ex);
            {
                // Dropdown button
                auto btn = std::make_unique<Button>(Rect{}, $(Icons::download << " Export"));
                {
                    auto menu = std::make_unique<Menu>(Rect{}, Menu::Orientation::Vertical);
                    auto add_item = [&menu](const SvgIconRef& icon, const std::string& label, auto&& click) {
                        Menu::Item item;
                        item.icon = icon;
                        item.label = label;
                        item.on_click = std::move(click);
                        menu->addItem(std::move(item));
                    };
                    add_item(Icons::file_text,  "Export as PDF", [] {});
                    add_item(Icons::image,      "Export as PNG", [] {});
                    add_item(Icons::download,   "Export as SVG", [] {});
                    btn->setDropdownMenu(std::move(menu));
                }
                row->addChild(std::move(btn));
            }
        }
    };

    // ── Page: Nav ──
    auto build_nav_page = [&](Container* c) {
        auto* ex = page_begin(c, "Nav",
            "Nav is a hierarchical navigation control with expandable groups, "
            "selectable items, and keyboard navigation. Resize by dragging the right edge.");
        if (!ex) return;

        auto inline_nav = std::make_unique<Nav>(Rect{0, 0, 220, 0});
        {
            Nav::NavGroup g;
            g.label = "Section A";
            g.id    = "eg_a";
            for (int i = 1; i <= 3; ++i) {
                Nav::NavItem it;
                it.label = "Item " + std::to_string(i);
                it.id    = "eg_a_" + std::to_string(i);
                g.items.push_back(std::move(it));
            }
            inline_nav->addGroup(std::move(g));
        }
        {
            Nav::NavGroup g;
            g.label = "Section B";
            g.id    = "eg_b";
            g.expanded = false;
            for (int i = 1; i <= 2; ++i) {
                Nav::NavItem it;
                it.label = "Sub " + std::to_string(i);
                it.id    = "eg_b_" + std::to_string(i);
                g.items.push_back(std::move(it));
            }
            inline_nav->addGroup(std::move(g));
        }
        inline_nav->setAutoSize(AutoSize::Height);
        inline_nav->setSelectedItem("eg_a_1");
        ex->addChild(std::move(inline_nav));
    };

    // ── Page: Menu ──
    auto build_menu_page = [&](Container* c) {
        auto* ex = page_begin(c, "Menu",
            "Menu supports horizontal menu bars and vertical dropdown menus with icons, "
            "submenus, and separators.");
        if (!ex) return;

        // Horizontal menu bar with dropdown menus and submenus
        {
            auto bar = std::make_unique<Menu>(Rect{}, Menu::Orientation::Horizontal);
            auto* bar_ptr = bar.get();

            auto add_sep = [](Menu* m) {
                Menu::Item sep;
                sep.separator = true;
                m->addItem(std::move(sep));
            };

            // ── File menu ──
            {
                Menu::Item file_item;
                file_item.icon  = Icons::file_text;
                file_item.label = "  File  ";

                auto dd = std::make_unique<Menu>(Rect{}, Menu::Orientation::Vertical);
                {
                    // "New" submenu
                    Menu::Item new_item;
                    new_item.icon  = Icons::file_plus;
                    new_item.label = "New";
                    {
                        auto sub = std::make_unique<Menu>(Rect{}, Menu::Orientation::Vertical);
                        auto add_sub = [&](auto icon, const std::string& l) {
                            Menu::Item it;
                            it.icon = icon; it.label = l; it.on_click = []{};
                            sub->addItem(std::move(it));
                        };
                        add_sub(Icons::file_text, "New File");
                        add_sub(Icons::folder,    "New Folder");
                        new_item.submenu = std::move(sub);
                    }
                    dd->addItem(std::move(new_item));
                }
                {
                    Menu::Item it;
                    it.icon = Icons::folder_open; it.label = "Open..."; it.on_click = []{};
                    dd->addItem(std::move(it));
                }
                add_sep(dd.get());
                {
                    Menu::Item it;
                    it.icon = Icons::save; it.label = "Save"; it.on_click = []{};
                    dd->addItem(std::move(it));
                }
                add_sep(dd.get());
                {
                    Menu::Item it;
                    it.icon = Icons::x; it.label = "Exit"; it.on_click = []{};
                    dd->addItem(std::move(it));
                }

                file_item.submenu = std::move(dd);
                bar_ptr->addItem(std::move(file_item));
            }

            // ── Edit menu ──
            {
                Menu::Item edit_item;
                edit_item.icon  = Icons::pencil;
                edit_item.label = "  Edit  ";

                auto dd = std::make_unique<Menu>(Rect{}, Menu::Orientation::Vertical);
                auto add_edit = [&](auto icon, const std::string& l) {
                    Menu::Item it;
                    it.icon = icon; it.label = l; it.on_click = []{};
                    dd->addItem(std::move(it));
                };
                add_edit(Icons::corner_down_left,  "Undo");
                add_edit(Icons::corner_down_right, "Redo");
                add_sep(dd.get());
                add_edit(Icons::crop,              "Cut");
                add_edit(Icons::copy,              "Copy");
                add_edit(Icons::clipboard_paste,   "Paste");

                edit_item.submenu = std::move(dd);
                bar_ptr->addItem(std::move(edit_item));
            }

            // ── View menu ──
            {
                Menu::Item view_item;
                view_item.icon  = Icons::eye;
                view_item.label = "  View  ";

                auto dd = std::make_unique<Menu>(Rect{}, Menu::Orientation::Vertical);
                auto add_view = [&](auto icon, const std::string& l) {
                    Menu::Item it;
                    it.icon = icon; it.label = l; it.on_click = []{};
                    dd->addItem(std::move(it));
                };
                add_view(Icons::zoom_in,  "Zoom In");
                add_view(Icons::zoom_out, "Zoom Out");
                add_sep(dd.get());
                add_view(Icons::maximize, "Fullscreen");

                view_item.submenu = std::move(dd);
                bar_ptr->addItem(std::move(view_item));
            }

            ex->addChild(std::move(bar));
        }
    };

    // ── Page: TextInput ──
    auto build_textinput_page = [&](Container* c) {
        auto* ex = page_begin(c, "Text Input",
            "TextInput supports single & multi-line editing, placeholders, "
            "prefix content (icon + text), scrollbars, and events.");
        if (!ex) return;

        {
            auto ti = std::make_unique<TextInput>(Rect{}, "Type here...");
            ti->setAutoSize(AutoSize::Height);
            ex->addChild(std::move(ti));
        }
        {
            auto ti = std::make_unique<TextInput>(Rect{}, "Search...");
            ti->setPrefix($(Icons::search << " "));
            ex->addChild(std::move(ti));
        }
        {
            auto ti = std::make_unique<TextInput>(Rect{0, 0, 0, 80.0f}, "Multi-line...");
            ti->setMultiline(true);
            ti->setGrowable(false);
            ex->addChild(std::move(ti));
        }
        {
            auto ti = std::make_unique<TextInput>(Rect{}, "With email icon and submit");
            ti->setPrefix($(Icons::at_sign << " "));
            ex->addChild(std::move(ti));
        }
    };

    // ── Page: Slider ──
    auto build_slider_page = [&](Container* c) {
        auto* ex = page_begin(c, "Slider",
            "Sliders select a value from a range. Supports custom format, "
            "value display, and change callbacks.");
        if (!ex) return;

        {
            auto sl = std::make_unique<Slider>(Rect{}, 0.5f);
            sl->setAutoSize(AutoSize::Height);
            sl->setShowValue(true);
            ex->addChild(std::move(sl));
        }
        {
            auto sl = std::make_unique<Slider>(Rect{}, 0.0f);
            sl->setAutoSize(AutoSize::Height);
            sl->setShowValue(true);
            sl->setValueFormat("{:.1f} units");
            ex->addChild(std::move(sl));
        }
    };

    // ── Page: ColorPicker ──
    auto build_colorpicker_page = [&](Container* c) {
        auto* ex = page_begin(c, "Color Picker",
            "ColorPicker provides RGB sliders and hex preview for choosing colors.");
        if (!ex) return;

        {
            auto cp = std::make_unique<ColorPicker>(Rect{}, Color{128, 128, 255, 255});
            cp->setAutoSize(AutoSize::Height);
            cp->setOnChange([](Color c) {
            });
            ex->addChild(std::move(cp));
        }
    };

    // ── Page: Checkbox ──
    auto build_checkbox_page = [&](Container* c) {
        auto* ex = page_begin(c, "Checkbox",
            "Checkboxes toggle a boolean state. Supports checked, unchecked, "
            "disabled states and change callbacks.");
        if (!ex) return;

        auto add_cb = [&](const std::string& label, bool checked, bool enabled) {
            auto cb = std::make_unique<Checkbox>(Rect{}, label);
            cb->setChecked(checked);
            if (!enabled) cb->setEnabled(false);
            cb->setAutoSize(AutoSize::Height);
            ex->addChild(std::move(cb));
        };
        add_cb("Unchecked", false, true);
        add_cb("Checked", true, true);
        add_cb("Disabled checked", true, false);

        {
            auto cb = std::make_unique<Checkbox>(Rect{}, "With callback");
            cb->setAutoSize(AutoSize::Height);
            ex->addChild(std::move(cb));
        }
    };

    // ── Page: RadioButton ──
    auto build_radio_page = [&](Container* c) {
        auto* ex = page_begin(c, "Radio Button",
            "Radio buttons allow single selection within a group. "
            "Only one radio in a group can be selected at a time.");
        if (!ex) return;

        // Group "size"
        {
            auto lbl = std::make_unique<Label>(Rect{}, "Size group:", Label::Align::Left);
            ex->addChild(std::move(lbl));

            auto* row = add_row(ex);
            auto add_rb = [&](const std::string& label, bool checked) {
                auto rb = std::make_unique<RadioButton>(Rect{}, label);
                rb->setGroup("size");
                rb->setChecked(checked);
                rb->setAutoSize(AutoSize::Height);
                row->addChild(std::move(rb));
            };
            add_rb("Small",  true);
            add_rb("Medium", false);
            add_rb("Large",  false);
        }

        // Group "color"
        {
            auto lbl = std::make_unique<Label>(Rect{}, "Color group:", Label::Align::Left);
            ex->addChild(std::move(lbl));

            auto* row = add_row(ex);
            auto add_rb = [&](const std::string& label, bool checked) {
                auto rb = std::make_unique<RadioButton>(Rect{}, label);
                rb->setGroup("color");
                rb->setChecked(checked);
                rb->setAutoSize(AutoSize::Height);
                row->addChild(std::move(rb));
            };
            add_rb("Red",   true);
            add_rb("Green", false);
            add_rb("Blue",  false);
            add_rb("Disabled", false);
        }
    };

    // ── Page: Switch ──
    auto build_switch_page = [&](Container* c) {
        auto* ex = page_begin(c, "Switch",
            "Switches toggle between on/off states. Supports enabled and disabled states.");
        if (!ex) return;

        {
            auto* row = add_row(ex);
            {
                auto sw = std::make_unique<Switch>(Rect{}, "Wi-Fi");
                sw->setAutoSize(AutoSize::Height);
                row->addChild(std::move(sw));
            }
            {
                auto sw = std::make_unique<Switch>(Rect{}, "Bluetooth");
                sw->setChecked(true);
                sw->setAutoSize(AutoSize::Height);
                row->addChild(std::move(sw));
            }
        }
        {
            auto* row = add_row(ex);
            {
                auto sw = std::make_unique<Switch>(Rect{}, "Airplane");
                sw->setEnabled(false);
                sw->setAutoSize(AutoSize::Height);
                row->addChild(std::move(sw));
            }
            {
                auto sw = std::make_unique<Switch>(Rect{}, "VPN");
                sw->setChecked(true);
                sw->setEnabled(false);
                sw->setAutoSize(AutoSize::Height);
                row->addChild(std::move(sw));
            }
        }
    };

    // ── Page: Info Dialog ──
    auto build_info_page = [&](Container* c) {
        auto* ex = page_begin(c, "Dialogs — Info",
            "Modal dialogs for user interaction. Supports Ok, OkCancel, YesNo, "
            "YesNoCancel button presets and fully custom buttons.");
        if (!ex) return;

        {
            auto btn = std::make_unique<Button>(Rect{}, $(Icons::badge_info << " Show Info"));
            btn->setOnClick([&win]() {
                win.showDialog("Information",
                    "A plain info dialog with an OK button.",
                    Window::DialogButtons::Ok);
            });
            ex->addChild(std::move(btn));
        }
    };

    // ── Page: Confirm Dialog ──
    auto build_confirm_page = [&](Container* c) {
        auto* ex = page_begin(c, "Dialogs — Confirm",
            "Confirm dialogs return the clicked button label as a string.");
        if (!ex) return;

        {
            auto btn = std::make_unique<Button>(Rect{}, $(Icons::badge_alert << " Yes / No"));
            btn->setOnClick([&win]() {
                win.showDialog("Confirm", "Are you sure?",
                    Window::DialogButtons::YesNo);
            });
            ex->addChild(std::move(btn));
        }
        {
            auto btn = std::make_unique<Button>(Rect{}, $(Icons::circle_question_mark << " Yes / No / Cancel"));
            btn->setOnClick([&win]() {
                win.showDialog("Choose", "Select an option:",
                    Window::DialogButtons::YesNoCancel);
            });
            ex->addChild(std::move(btn));
        }
    };

    // ── Page: Theme Manager ──
    auto build_themes_page = [&](Container* c) {
        auto* ex = page_begin(c, "Theme Manager",
            "Switch between built-in themes or apply a custom YAML theme. "
            "The accent color can also be changed via the Color Picker control.");
        if (!ex) return;

        // Theme preset buttons
        {
            auto* row = add_row(ex);
            auto add_btn = [&row](const SvgIconRef& icon, const std::string& label, auto&& cb) {
                auto btn = std::make_unique<Button>(Rect{}, $(icon << " " << label));
                btn->setOnClick(std::move(cb));
                row->addChild(std::move(btn));
            };
            add_btn(Icons::sun,  "Light",     [&win]() { win.setTheme(Theme::defaultLight()); });
            add_btn(Icons::moon, "Dark",      [&win]() { win.setTheme(Theme::defaultDark()); });
            add_btn(Icons::brush, "Solarized", [&win]() { win.setTheme(Theme::solarized()); });
        }

        // Label for custom theme section
        {
            auto lbl = std::make_unique<Label>(Rect{}, "Custom YAML Theme:", Label::Align::Left);
            ex->addChild(std::move(lbl));
        }

        // Multiline TextInput with default YAML theme content
        auto ti = std::make_unique<TextInput>(Rect{0, 0, 0, 120.0f}, "Paste your YAML theme...");
        ti->setMultiline(true);
        ti->setGrowable(false);
        {
            // Default YAML content — a warm custom theme
            const char* default_yaml =
                "name: custom\n"
                "font_family: default\n"
                "colors:\n"
                "  background: \"#FF1A1A2E\"\n"
                "  surface: \"#FF16213E\"\n"
                "  on_surface: \"#FFE0E0FF\"\n"
                "  on_surface_mute: \"#FF9090C0\"\n"
                "  accent: \"#FFE94560\"\n"
                "  accent_pressed: \"#FFC03050\"\n"
                "  border: \"#FF2A2A4E\"\n"
                "  disabled: \"#FF4A4A6E\"\n"
                "metrics:\n"
                "  corner_radius: 10\n"
                "  border_width: 1\n"
                "  text_size: 14\n"
                "  button_padding_x: 16\n"
                "  button_padding_y: 10\n"
                "shadows:\n"
                "  shadow:\n"
                "    color: \"#44000000\"\n"
                "    blur: 12\n"
                "    offset_x: 0\n"
                "    offset_y: 2\n"
                "  shadow_hover:\n"
                "    color: \"#66000000\"\n"
                "    blur: 16\n"
                "    offset_x: 0\n"
                "    offset_y: 3\n";
            ti->setText(default_yaml);
        }
        TextInput* ti_ptr = ti.get();
        ex->addChild(std::move(ti));

        // Apply button
        {
            auto btn = std::make_unique<Button>(Rect{}, $(Icons::check << " Apply Custom Theme"));
            btn->setOnClick([&win, ti_ptr]() {
                const std::string& yaml_text = ti_ptr->text();
                if (yaml_text.empty()) {
                    return;
                }
                Theme custom = Theme::fromYaml(yaml_text);
                win.setTheme(custom);
            });
            ex->addChild(std::move(btn));
        }
    };

    // ── Page: Custom Dialog ──
    auto build_custom_page = [&](Container* c) {
        auto* ex = page_begin(c, "Dialogs — Custom",
            "Custom dialogs support inline content with icons for both the message "
            "and each button label.");
        if (!ex) return;

        {
            auto btn = std::make_unique<Button>(Rect{}, $(Icons::cog << " Custom Dialog"));
            btn->setOnClick([&win]() {
                win.showDialog("Custom Dialog",
                    $(Icons::check << " Operation completed successfully"),
                    {$(Icons::check << " OK"),
                     $(Icons::x << " Cancel"),
                     $(Icons::circle_question_mark << " Help")});
            });
            ex->addChild(std::move(btn));
        }
    };

    // ── Page: Scroll ──
    auto build_scroll_page = [&](Container* c) {
        auto* ex = page_begin(c, "Scroll",
            "Scrollable containers clip overflowing content and show scroll indicators. "
            "Use the mouse wheel to scroll.");
        if (!ex) return;

        // Let ex fill remaining vertical space via stretch weight
        ex->setAutoSize(AutoSize::Width);

        // Fixed-height scrollable area
        {
            auto scroll = std::make_unique<Container>(Rect{0, 0, 0, 220.0f});
            scroll->setScrollable(true);
            scroll->setAutoSize(AutoSize::Disabled);
            auto vl = std::make_unique<VerticalLayout>();
            vl->setPadding(8.0f);
            vl->setSpacing(6.0f);
            scroll->setLayout(std::move(vl));

            for (int i = 1; i <= 20; ++i) {
                auto row = std::make_unique<Container>(Rect{});
                auto hl = std::make_unique<HorizontalLayout>();
                hl->setSpacing(8.0f);
                row->setLayout(std::move(hl));
                row->setAutoSize(AutoSize::Height);

                auto num = std::make_unique<Label>(
                    Rect{}, std::to_string(i) + ".",
                    Label::Align::Left);
                num->setAutoSize(AutoSize::Both);
                row->addChild(std::move(num));

                auto txt = std::make_unique<Label>(
                    Rect{}, "This is scrollable item number "
                        + std::to_string(i)
                        + " — it scrolls when the content overflows the container.",
                    Label::Align::Left);
                txt->setWordWrap(true);
                txt->setAutoSize(AutoSize::Both);
                row->addChild(std::move(txt));

                scroll->addChild(std::move(row));
            }

            ex->addChild(std::move(scroll));
        }
    };

    // ── Page: Image ──
    auto build_image_page = [&](Container* c) {
        auto* ex = page_begin(c, "Image",
            "Images display raster files (PNG, JPG, etc.) using platform-native "
            "decoders. Supports Fit, Fill, Stretch, and None scale modes.");
        if (!ex) return;

        // Generate a test image — 128x128 gradient (red→blue)
        Image* img_ptr = nullptr;
        {
            constexpr int sz = 128;
            std::vector<unsigned char> pixels(sz * sz * 4);
            for (int y = 0; y < sz; ++y) {
                for (int x = 0; x < sz; ++x) {
                    const int i = (y * sz + x) * 4;
                    const float t = static_cast<float>(x) / static_cast<float>(sz);
                    pixels[i + 0] = static_cast<unsigned char>((1.0f - t) * 255.0f);  // R
                    pixels[i + 1] = static_cast<unsigned char>(t * 128.0f);            // G
                    pixels[i + 2] = static_cast<unsigned char>(t * 255.0f);            // B
                    pixels[i + 3] = 255;                                                // A
                }
            }

            auto lbl = std::make_unique<Label>(Rect{}, "From generated RGBA pixels:", Label::Align::Left);
            ex->addChild(std::move(lbl));

            auto img = std::make_unique<Image>(
                Rect{0, 0, 200, 200}, sz, sz, pixels.data());
            img->setScaleMode(ScaleMode::Fit);
            img->setBorder(true);
            img_ptr = img.get();
            ex->addChild(std::move(img));
        }

        // Scale mode buttons
        {
            auto* row = add_row(ex);
            {
                auto lbl = std::make_unique<Label>(Rect{}, "Scale modes:", Label::Align::Left);
                row->addChild(std::move(lbl));
            }
            {
                auto btn = std::make_unique<Button>(Rect{}, "Fit");
                btn->setAutoSize(AutoSize::Height);
                btn->setOnClick([img_ptr]() { img_ptr->setScaleMode(ScaleMode::Fit); });
                row->addChild(std::move(btn));
            }
            {
                auto btn = std::make_unique<Button>(Rect{}, "Fill");
                btn->setAutoSize(AutoSize::Height);
                btn->setOnClick([img_ptr]() { img_ptr->setScaleMode(ScaleMode::Fill); });
                row->addChild(std::move(btn));
            }
            {
                auto btn = std::make_unique<Button>(Rect{}, "Stretch");
                btn->setAutoSize(AutoSize::Height);
                btn->setOnClick([img_ptr]() { img_ptr->setScaleMode(ScaleMode::Stretch); });
                row->addChild(std::move(btn));
            }
            {
                auto btn = std::make_unique<Button>(Rect{}, "None");
                btn->setAutoSize(AutoSize::Height);
                btn->setOnClick([img_ptr]() { img_ptr->setScaleMode(ScaleMode::None); });
                row->addChild(std::move(btn));
            }
            {
                auto sep = std::make_unique<Label>(Rect{}, "");
                sep->setAutoSize(AutoSize::Height);
                row->addChild(std::move(sep));
            }
            {
                auto sw = std::make_unique<Switch>(Rect{}, "Border");
                sw->setChecked(true);
                sw->setAutoSize(AutoSize::Height);
                sw->setOnChange([img_ptr](bool on) { img_ptr->setBorder(on); });
                row->addChild(std::move(sw));
            }
        }

        {
            auto lbl = std::make_unique<Label>(Rect{},
                "File loading: use setImage(\"path/to/image.png\") to load from disk. "
                "Platform decoders handle PNG, JPG, GIF, BMP, TIFF, and more.",
                Label::Align::Left);
            ex->addChild(std::move(lbl));
        }
    };

    // ── Page: WebView ──
#ifdef FLUFFY_ENABLE_WEBVIEW
    auto build_webview_page = [&](Container* c) {
        auto* ex = page_begin(c, "WebView",
            "WebView displays web content via native overlay (WKWebView / WebKitGTK / WebView2) — "
            "semantic postMessage, JS evaluation, overlay 60fps.");
        if (!ex) return;

        auto status = std::make_unique<Label>(Rect{}, "Ready — https://example.com", Label::Align::Left);
        status->setAutoSize(AutoSize::Height);
        Label* status_ptr = status.get();
        ex->addChild(std::move(status));

        auto wv = std::make_unique<WebView>(Rect{0, 0, 680, 320}, "https://example.com");
        wv->setOnMessage([status_ptr](const std::string& json, WebView&) {
            if (status_ptr) status_ptr->setText("Message: " + json);
        });
        wv->setOnNavigation([](const std::string& url) { (void)url; return true; });
        WebView* wv_ptr = wv.get();
        ex->addChild(std::move(wv));

        // The page-side bridge call is platform-native: WKWebView on macOS,
        // WebKitGTK on Linux, WebView2 on Windows. Fluffy injects no shims.
#if defined(_WIN32)
        const std::string post_msg = "window.chrome.webview.postMessage";
#else
        const std::string post_msg = "window.webkit.messageHandlers.fluffy.postMessage";
#endif

        auto* row = add_row(ex);
        {
            auto btn = std::make_unique<Button>(Rect{}, "Navigate");
            btn->setOnClick([wv_ptr, status_ptr]() {
                if (!wv_ptr) return;
                wv_ptr->navigate("https://www.google.com");
                if (status_ptr) status_ptr->setText("Navigated to https://www.google.com");
            });
            row->addChild(std::move(btn));
        }
        {
            auto btn = std::make_unique<Button>(Rect{}, "Load HTML");
            btn->setOnClick([wv_ptr, status_ptr, post_msg]() {
                if (!wv_ptr) return;
                wv_ptr->setHtml(
                    "<html><body style='font-family:system-ui;padding:24px;background:#F3F3F3'>"
                    "<h2 style='color:#0078D4'>Hello from Fluffy</h2>"
                    "<p>Native WebView overlay — WKWebView, WebKitGTK, or WebView2.</p>"
                    "<button onclick=\"" + post_msg +
                    "(JSON.stringify({type:'buy',id:42}))\" "
                    "style='padding:8px 12px;background:#0078D4;color:white;border:none;border-radius:4px'>PostMessage</button>"
                    "</body></html>");
                if (status_ptr) status_ptr->setText("Loaded HTML — Hello from Fluffy");
            });
            row->addChild(std::move(btn));
        }
        {
            auto btn = std::make_unique<Button>(Rect{}, "Eval JS");
            btn->setOnClick([wv_ptr, status_ptr]() {
                if (!wv_ptr) return;
                wv_ptr->evalJS("document.title", [status_ptr](const std::string& res) {
                    if (status_ptr) status_ptr->setText("Title: " + res);
                });
            });
            row->addChild(std::move(btn));
        }
        {
            auto hint = std::make_unique<Label>(Rect{},
                "WebView is a native overlay (WKWebView/WebKitGTK/WebView2) — not clipped by scroll, always topmost rect. "
                "Placeholder shows when native not yet created.",
                Label::Align::Left);
            hint->setWordWrap(true);
            hint->setAutoSize(AutoSize::Both);
            ex->addChild(std::move(hint));
        }
    };
#else
    auto build_webview_page = [&](Container* c) {
        auto* ex = page_begin(c, "WebView (disabled)",
            "WebView overlay disabled — rebuild with -DFLUFFY_ENABLE_WEBVIEW=ON");
        (void)ex;
    };
#endif

    // ── Page: ComboBox ──
    auto build_combobox_page = [&](Container* c) {
        auto* ex = page_begin(c, "ComboBox",
            "ComboBox selects one item from a dropdown list. Click to open, type to filter, arrows to navigate.");
        if (!ex) return;
        auto cb = std::make_unique<ComboBox>(Rect{0, 0, 240, 0});
        cb->setItems({"Apple", "Banana", "Cherry", "Date", "Elderberry", "Fig", "Grape"});
        cb->setSelectedIndex(1);
        auto status = std::make_unique<Label>(Rect{}, "Selected: Banana", Label::Align::Left);
        status->setAutoSize(AutoSize::Height);
        Label* status_ptr = status.get();
        cb->setOnChange([status_ptr](int idx, const std::string& v) {
            if (status_ptr) status_ptr->setText("Selected " + std::to_string(idx) + ": " + v);
        });
        ex->addChild(std::move(cb));
        ex->addChild(std::move(status));
        auto hint = std::make_unique<Label>(Rect{}, "Tip: Use Up/Down, Enter/Space to open.", Label::Align::Left);
        hint->setWordWrap(true);
        ex->addChild(std::move(hint));
    };

    // ── Page: ListView ──
    auto build_listview_page = [&](Container* c) {
        auto* ex = page_begin(c, "ListView",
            "ListView shows a scrollable list with single selection. Click or use Up/Down/Home/End.");
        if (!ex) return;
        auto lv = std::make_unique<ListView>(Rect{0, 0, 300, 160});
        std::vector<std::string> items;
        for (int i = 1; i <= 30; ++i) items.push_back("Item " + std::to_string(i) + " — example row");
        lv->setItems(items);
        lv->setSelected(2);
        auto status = std::make_unique<Label>(Rect{}, "Selected: Item 3", Label::Align::Left);
        status->setAutoSize(AutoSize::Height);
        Label* status_ptr = status.get();
        lv->setOnSelect([status_ptr](int idx, const std::string& v) {
            if (status_ptr) status_ptr->setText("Selected " + std::to_string(idx) + ": " + v);
        });
        ex->addChild(std::move(lv));
        ex->addChild(std::move(status));
    };

    // ── Page: TabView ──
    auto build_tabview_page = [&](Container* c) {
        auto* ex = page_begin(c, "TabView",
            "TabView switches between panels. Click a tab header to select.");
        if (!ex) return;
        auto tv = std::make_unique<TabView>(Rect{0, 0, 0, 260});
        tv->addTab("Overview", "tab_overview");
        tv->addTab("Details", "tab_details");
        tv->addTab("Settings", "tab_settings");
        TabView* tv_ptr = tv.get();
        auto status = std::make_unique<Label>(Rect{}, "Tab: Overview", Label::Align::Left);
        status->setAutoSize(AutoSize::Height);
        Label* status_ptr = status.get();
        // Helper to populate content per tab
        auto populate = [tv_ptr](int idx) {
            if (!tv_ptr) return;
            auto* cont = tv_ptr->content();
            cont->removeAllChildren();
            if (idx == 0) {
                auto lbl = std::make_unique<Label>(Rect{}, "Overview content — add your widgets to TabView::content().", Label::Align::Left);
                lbl->setWordWrap(true);
                cont->addChild(std::move(lbl));
                auto btn = std::make_unique<Button>(Rect{}, "Action in Overview");
                cont->addChild(std::move(btn));
            } else if (idx == 1) {
                auto lbl = std::make_unique<Label>(Rect{}, "Details panel — here you could show a form or stats.", Label::Align::Left);
                lbl->setWordWrap(true);
                cont->addChild(std::move(lbl));
                auto ti = std::make_unique<TextInput>(Rect{}, "Details input...");
                cont->addChild(std::move(ti));
            } else {
                auto lbl = std::make_unique<Label>(Rect{}, "Settings panel — toggles and options.", Label::Align::Left);
                lbl->setWordWrap(true);
                cont->addChild(std::move(lbl));
                auto sw = std::make_unique<Switch>(Rect{}, "Enable feature");
                cont->addChild(std::move(sw));
            }
        };
        populate(0);
        tv->setOnSelect([status_ptr, tv_ptr, populate](int idx, const std::string& id) {
            if (status_ptr) status_ptr->setText("Selected " + std::to_string(idx) + ": " + id);
            populate(idx);
        });
        ex->addChild(std::move(tv));
        ex->addChild(std::move(status));
    };

    // ── Page: Grid ──
    auto build_grid_page = [&](Container* c) {
        auto* ex = page_begin(c, "Grid",
            "GridLayout arranges children in equal cells — cols × rows, e.g. 2×3, 3×2. Useful for forms/dashboards.");
        if (!ex) return;
        auto grid = std::make_unique<Container>(Rect{0, 0, 0, 180});
        auto gl = std::make_unique<GridLayout>(3);
        gl->setSpacing(8.0f);
        gl->setPadding(4.0f);
        grid->setLayout(std::move(gl));
        grid->setAutoSize(AutoSize::Disabled);
        for (int i = 1; i <= 6; ++i) {
            auto cell = std::make_unique<Button>(Rect{}, "Cell " + std::to_string(i));
            grid->addChild(std::move(cell));
        }
        ex->addChild(std::move(grid));
        auto hint = std::make_unique<Label>(Rect{}, "Grid cols=3 (auto rows=2). Change via GridLayout::setCols/setRows.", Label::Align::Left);
        hint->setWordWrap(true);
        ex->addChild(std::move(hint));
    };

    // ── Page: ProgressRing ──
    auto build_progress_page = [&](Container* c) {
        auto* ex = page_begin(c, "ProgressRing",
            "ProgressRing shows determinate (0..1) or indeterminate spinner. Tick drives spinner.");
        if (!ex) return;
        auto* row = add_row(ex);
        auto pr_indet = std::make_unique<ProgressRing>(Rect{0, 0, 48, 48}, -1.0f);
        row->addChild(std::move(pr_indet));
        auto pr25 = std::make_unique<ProgressRing>(Rect{0, 0, 48, 48}, 0.25f);
        row->addChild(std::move(pr25));
        auto pr50 = std::make_unique<ProgressRing>(Rect{0, 0, 48, 48}, 0.5f);
        row->addChild(std::move(pr50));
        auto pr75 = std::make_unique<ProgressRing>(Rect{0, 0, 48, 48}, 0.75f);
        row->addChild(std::move(pr75));
        auto lbl = std::make_unique<Label>(Rect{}, "Indeterminate + 25% 50% 75% — setValue(0..1) or setIndeterminate(true)", Label::Align::Left);
        lbl->setWordWrap(true);
        ex->addChild(std::move(lbl));
    };

    // ── Page: Constraints ──
    auto build_constraints_page = [&](Container* c) {
        auto* ex = page_begin(c, "Constraints",
            "Anchor constraints pin/center/size widgets: left/right/top/bottom/width/height/centerX/centerY. Solved every frame; constraints win over layout, no layout block needed.");
        if (!ex) return;
        auto box = std::make_unique<Container>(Rect{0, 0, 0, 250});
        box->setAutoSize(AutoSize::Disabled);
        ex->addChild(std::move(box));
        Container* demo = static_cast<Container*>(ex->child(ex->childCount() - 1));

        auto title = std::make_unique<Label>(Rect{}, "Pinned header", Label::Align::Left);
        Label* titlePtr = title.get();
        demo->addChild(std::move(title));
        constrain(*titlePtr).leftToParent(12.0f).topToParent(8.0f).widthPercentOfParent(1.0f, -24.0f);

        auto sub = std::make_unique<Label>(Rect{}, "Chained below header (prev.bottom+8)", Label::Align::Left);
        Label* subPtr = sub.get();
        demo->addChild(std::move(sub));
        constrain(*subPtr).leftToParent(12.0f).top(Anchor::Bottom, titlePtr, 1.0f, 8.0f).widthPercentOfParent(1.0f, -24.0f);

        auto centered = std::make_unique<Button>(Rect{}, "Centered 50% wide");
        Button* centeredPtr = centered.get();
        demo->addChild(std::move(centered));
        constrain(*centeredPtr).centerXToParent().centerYToParent(24.0f).widthPercentOfParent(0.5f).heightConstant(36.0f);

        auto phase = std::make_shared<int>(0);
        auto cycle = std::make_unique<Button>(Rect{}, "Click to cycle anchor");
        Button* cyclePtr = cycle.get();
        demo->addChild(std::move(cycle));
        constrain(*cyclePtr).leftToParent(12.0f).bottomToParent(-12.0f).widthConstant(200.0f).heightConstant(36.0f);
        cyclePtr->setOnClick([cyclePtr, phase]() {
            *phase = (*phase + 1) % 3;
            constrain(*cyclePtr).clear();
            if (*phase == 0) constrain(*cyclePtr).leftToParent(12.0f).centerYToParent(72.0f).widthConstant(200.0f).heightConstant(36.0f);
            else if (*phase == 1) constrain(*cyclePtr).centerXToParent().centerYToParent(72.0f).widthConstant(200.0f).heightConstant(36.0f);
            else constrain(*cyclePtr).rightToParent(-12.0f).centerYToParent(72.0f).widthConstant(200.0f).heightConstant(36.0f);
        });

        auto hint = std::make_unique<Label>(Rect{}, "C++: constrain(*w).leftToParent(12). YAML: constraints: { left: \"parent.left+12\" } — same solver, no layout block.", Label::Align::Left);
        hint->setWordWrap(true);
        ex->addChild(std::move(hint));
    };

    // ── Page: Tooltip ──
    auto build_tooltip_page = [&](Container* c) {
        auto* ex = page_begin(c, "Tooltip",
            "Tooltip shows on hover (auto-hides after 2.5s or on leave). Use Tooltip::showFor or fluffy::setTooltip.");
        if (!ex) return;
        auto hoverBtn = std::make_unique<Button>(Rect{}, "Hover me");
        // Default hover tooltip via userdata — Window will auto-show
        hoverBtn->userdata() = "Tooltip on hover — hello!";
        // Also set via helper for example
        setTooltip(hoverBtn.get(), "Tooltip on hover — hello!");
        ex->addChild(std::move(hoverBtn));

        auto btn = std::make_unique<Button>(Rect{}, "Click for tooltip");
        Button* btn_ptr = btn.get();
        btn->setOnClick([btn_ptr]() {
            if (!btn_ptr || !btn_ptr->window()) return;
            Tooltip::showFor(btn_ptr, "This is a tooltip — native popup, follows anchor", *btn_ptr->window());
        });
        ex->addChild(std::move(btn));
        auto hint = std::make_unique<Label>(Rect{}, "Hover 'Hover me' (auto-hides) or click 'Click for tooltip' (auto-hides after 2.5s).", Label::Align::Left);
        hint->setWordWrap(true);
        ex->addChild(std::move(hint));
    };

    // ── Page: Table ──
    static Table* g_table_ptr = nullptr;
    auto build_table_page = [&](Container* c) {
        auto* ex = page_begin(c, "Table",
            "Table shows rows and columns with editable cells. Click to select, double-click or Enter to edit, arrows to navigate.");
        if (!ex) return;
        std::vector<Table::Column> cols = {
            {"Name", 160, true, Table::Column::Kind::Text},
            {"Active", 70, true, Table::Column::Kind::Bool},
            {"Age", 60, true, Table::Column::Kind::Text},
            {"City", 140, true, Table::Column::Kind::Choice},
            {"Role", 140, true, Table::Column::Kind::Text},
            {"Country", 140, true, Table::Column::Kind::Text}
        };
        cols[3].choices = {"Paris","London","Berlin","Madrid","Rome","Oslo","Vienna","Prague","Warsaw","Lisbon","Paris","London"};
        // Need to set headerInline for glyphs after choices
        auto tbl = std::make_unique<Table>(Rect{0, 0, 0, 240});
        tbl->setColumns(cols);
        // Glyphs only in headers — headers customizable
        tbl->setHeader(0, Inline{} << Icons::user << " Name");
        tbl->setHeader(1, Inline{} << Icons::check << " Active");
        tbl->setHeader(2, Inline{} << Icons::tag << " Age");
        tbl->setHeader(3, Inline{} << Icons::map_pin << " City");
        tbl->setHeader(4, Inline{} << Icons::briefcase << " Role");
        tbl->setHeader(5, Inline{} << Icons::globe << " Country");
        tbl->setRows({
            {"Alice", "true", "28", "Paris", "Designer", "France"},
            {"Bob", "false", "34", "London", "Developer", "UK"},
            {"Carol", "true", "29", "Berlin", "Manager", "Germany"},
            {"Dave", "false", "41", "Madrid", "Developer", "Spain"},
            {"Eve", "true", "26", "Rome", "Designer", "Italy"},
            {"Frank", "false", "33", "Oslo", "QA", "Norway"},
            {"Grace", "true", "31", "Vienna", "Designer", "Austria"},
            {"Heidi", "false", "27", "Prague", "Developer", "Czech"},
            {"Ivan", "true", "36", "Warsaw", "Manager", "Poland"},
            {"Judy", "false", "30", "Lisbon", "Designer", "Portugal"},
            {"Karl", "true", "45", "Dublin", "QA", "Ireland"},
            {"Liam", "false", "29", "Brussels", "Developer", "Belgium"},
            {"Mona", "true", "32", "Athens", "Designer", "Greece"},
            {"Nina", "false", "28", "Helsinki", "Manager", "Finland"},
            {"Oscar", "true", "38", "Stockholm", "Developer", "Sweden"},
            {"Paul", "false", "26", "Copenhagen", "QA", "Denmark"},
            {"Quinn", "true", "35", "Reykjavik", "Designer", "Iceland"},
            {"Rita", "false", "29", "Tallinn", "Manager", "Estonia"},
            {"Sam", "true", "40", "Vilnius", "Developer", "Lithuania"},
            {"Tina", "false", "27", "Riga", "Designer", "Latvia"},
        });
        tbl->setSelected(0);
        g_table_ptr = tbl.get();
        auto status = std::make_unique<Label>(Rect{}, "Selected: Alice — Designer", Label::Align::Left);
        status->setAutoSize(AutoSize::Height);
        Label* status_ptr = status.get();
        tbl->setOnSelect([status_ptr](int r, int c) {
            (void)r; (void)c;
            if (status_ptr) status_ptr->setText("Selected row " + std::to_string(r) + " col " + std::to_string(c));
        });
        tbl->setOnCellChange([status_ptr](int r, int c, const std::string& v) {
            if (status_ptr) status_ptr->setText("Cell [" + std::to_string(r) + "," + std::to_string(c) + "] = " + v);
        });
        ex->addChild(std::move(tbl));
        ex->addChild(std::move(status));
        auto hint2 = std::make_unique<Label>(Rect{}, "Headers have glyphs (setHeader) and custom bk/text/font; cells have custom font/bg — double-click/Enter to edit.", Label::Align::Left);
        hint2->setWordWrap(true);
        ex->addChild(std::move(hint2));
    };

    // ── Page: Font ──
    auto build_font_page = [&](Container* c) {
        auto* ex = page_begin(c, "Font",
            "System fonts with Latin character support. Select a font to preview it below.");
        if (!ex) return;

        // Preview labels (created first so we can capture pointers)
        Label* preview1 = nullptr;
        Label* preview2 = nullptr;
        Label* preview3 = nullptr;
        {
            auto lbl = std::make_unique<Label>(Rect{},
                "The quick brown fox jumps over the lazy dog",
                Label::Align::Left);
            lbl->setTextSize(16.0f);
            lbl->setAutoSize(AutoSize::Height);
            preview1 = lbl.get();
            ex->addChild(std::move(lbl));
        }
        {
            auto lbl = std::make_unique<Label>(Rect{},
                "ABCDEFGHIJKLMNOPQRSTUVWXYZ 0123456789",
                Label::Align::Left);
            lbl->setTextSize(14.0f);
            lbl->setAutoSize(AutoSize::Height);
            preview2 = lbl.get();
            ex->addChild(std::move(lbl));
        }
        {
            auto lbl = std::make_unique<Label>(Rect{},
                "abcdefghijklmnopqrstuvwxyz !@#$%^&*()",
                Label::Align::Left);
            lbl->setTextSize(14.0f);
            lbl->setAutoSize(AutoSize::Height);
            preview3 = lbl.get();
            ex->addChild(std::move(lbl));
        }

        // Spacer
        {
            auto spacer = std::make_unique<Label>(Rect{}, "");
            spacer->setTextSize(12.0f);
            spacer->setAutoSize(AutoSize::Height);
            ex->addChild(std::move(spacer));
        }

        // Font dropdown button
        {
            auto menu = std::make_unique<Menu>(Rect{}, Menu::Orientation::Vertical);
            if (fonts_ready) {
                std::lock_guard<std::mutex> lock(font_mutex);
                std::set<std::string> seen;
                for (const auto& path : latin_fonts) {
                    auto name = fs::path(path).stem().string();
                    if (!seen.insert(name).second) continue;
                    Menu::Item item;
                    item.label = name;
                    item.on_click = [&win, preview1, preview2, preview3, path, name]() {
                        if (win.loadSystemFont(name)) {
                            const std::string spec = "file:" + path;
                            preview1->setFont(spec);
                            preview2->setFont(spec);
                            preview3->setFont(spec);
                        }
                    };
                    menu->addItem(std::move(item));
                }
            } else {
                Menu::Item loading;
                loading.label = "Scanning fonts...";
                loading.enabled = false;
                menu->addItem(std::move(loading));
            }

            auto btn = std::make_unique<Button>(Rect{}, "System Fonts");
            btn->setAutoSize(AutoSize::Both);
            btn->setDropdownMenu(std::move(menu));
            auto center = std::make_unique<Container>(Rect{});
            center->setAutoSize(AutoSize::Height);
            auto cl = std::make_unique<HorizontalLayout>();
            cl->justify(Justify::Center);
            center->setLayout(std::move(cl));
            center->addChild(std::move(btn));
            ex->addChild(std::move(center));
        }
    };
    Nav* nav_ptr = nav.get();
    nav_ptr->computeMinWidth(cfg.theme, win);
    nav_ptr->collapseAll();
    root.addChild(std::move(nav));

    // ── Content area (right pane — shows widget example pages) ──
    auto content = std::make_unique<Container>(Rect{212, 0, 708, 640});
    Container* content_ptr = content.get();
    root.addChild(std::move(content));

    // ── Helper: reflow children based on Nav width ──
    auto reflow = [&root, nav_ptr, content_ptr]() {
        if (root.childCount() < 2) return;
        const float spacing = 12.0f;
        const float nav_w = nav_ptr->bounds().w;
        const float rx = root.bounds().x;
        const float ry = root.bounds().y;
        const float rh = root.bounds().h;
        const float content_x = nav_w + spacing;
        const float content_w = root.bounds().w - content_x;
        nav_ptr->setBounds(Rect{rx, ry, nav_w, rh});
        content_ptr->setBounds(Rect{rx + content_x, ry, content_w, rh});
    };

    // ── Connect Nav selection to page building ──
    // We use a map of page id -> builder function
    auto& content_ref = *content_ptr;

    nav_ptr->setOnSelect([&content_ref, &win, &reflow,
                           build_label_page, build_button_page, build_nav_page, build_menu_page,
                           build_textinput_page, build_slider_page, build_colorpicker_page,
                           build_checkbox_page, build_radio_page, build_switch_page,
                           build_info_page, build_confirm_page, build_custom_page,
                           build_scroll_page, build_image_page, build_webview_page,
                            build_combobox_page, build_listview_page, build_tabview_page, build_grid_page, build_progress_page, build_tooltip_page, build_table_page,
                            build_font_page, build_themes_page, build_constraints_page]
                          (const std::string& id) {
        // Clear focus before destroying children (avoids use-after-free of focusWidget_)
        win.clearFocus();

        // Clear content area
        content_ref.removeAllChildren();

        // Build the selected page
        if (id == "page_label")       { build_label_page(&content_ref); }
        else if (id == "page_button") { build_button_page(&content_ref); }
        else if (id == "page_nav")    { build_nav_page(&content_ref); }
        else if (id == "page_menu")   { build_menu_page(&content_ref); }
        else if (id == "page_textinput")   { build_textinput_page(&content_ref); }
        else if (id == "page_slider")      { build_slider_page(&content_ref); }
        else if (id == "page_colorpicker") { build_colorpicker_page(&content_ref); }
        else if (id == "page_checkbox")    { build_checkbox_page(&content_ref); }
        else if (id == "page_radio")       { build_radio_page(&content_ref); }
        else if (id == "page_switch")      { build_switch_page(&content_ref); }
        else if (id == "page_info")    { build_info_page(&content_ref); }
        else if (id == "page_confirm") { build_confirm_page(&content_ref); }
        else if (id == "page_custom")  { build_custom_page(&content_ref); }
        else if (id == "page_scroll") { build_scroll_page(&content_ref); }
        else if (id == "page_image") { build_image_page(&content_ref); }
        else if (id == "page_webview") { build_webview_page(&content_ref); }
        else if (id == "page_combobox") { build_combobox_page(&content_ref); }
        else if (id == "page_listview") { build_listview_page(&content_ref); }
        else if (id == "page_tabview") { build_tabview_page(&content_ref); }
        else if (id == "page_grid") { build_grid_page(&content_ref); }
        else if (id == "page_progress") { build_progress_page(&content_ref); }
        else if (id == "page_tooltip") { build_tooltip_page(&content_ref); }
        else if (id == "page_table") { build_table_page(&content_ref); }
        else if (id == "page_font") { build_font_page(&content_ref); }
        else if (id == "page_themes") { build_themes_page(&content_ref); }
        else if (id == "page_constraints") { build_constraints_page(&content_ref); }

        // Reflow to ensure proper sizing after layout changes
        reflow();
    });

    // ── Connect Nav resize to update content ──
    nav_ptr->setOnWidthChange([reflow](float) { reflow(); });

    // ── Handle window resize ──
    win.setOnResize([reflow](int, int) { reflow(); });

    // ── Initial reflow ──
    reflow();

    // ── Select default page ──
    nav_ptr->setSelectedItem("page_label");

    win.run();
    return 0;
}
