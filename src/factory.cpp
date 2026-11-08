// src/factory.cpp
//
// WidgetFactory / LayoutFactory — flat-array registries using function pointers.

#include <fluffy/factory.hpp>
#include <fluffy/button.hpp>
#include <fluffy/checkbox.hpp>
#include <fluffy/color_picker.hpp>
#include <fluffy/combobox.hpp>
#include <fluffy/container.hpp>
#include <fluffy/label.hpp>
#include <fluffy/layout.hpp>
#include <fluffy/listview.hpp>
#include <fluffy/menu.hpp>
#include <fluffy/modal_window.hpp>
#include <fluffy/nav.hpp>
#include <fluffy/progress_ring.hpp>
#include <fluffy/radiobutton.hpp>
#include <fluffy/slider.hpp>
#include <fluffy/switch.hpp>
#include <fluffy/table.hpp>
#include <fluffy/tabview.hpp>
#include <fluffy/textinput.hpp>
#include <fluffy/tooltip.hpp>
#include <fluffy/webview.hpp>
#include <cstring>
#include <string_view>
namespace fluffy {
namespace {
struct WidgetEntry { std::string_view name; WidgetFactory::Creator creator; };
struct LayoutEntry { std::string_view name; LayoutFactory::Creator creator; };

WidgetEntry kWidgets[] = {
    {"Button",       []() -> std::unique_ptr<Widget> { return std::make_unique<Button>(); }},
    {"Checkbox",     []() -> std::unique_ptr<Widget> { return std::make_unique<Checkbox>(); }},
    {"ColorPicker",  []() -> std::unique_ptr<Widget> { return std::make_unique<ColorPicker>(); }},
    {"ComboBox",     []() -> std::unique_ptr<Widget> { return std::make_unique<ComboBox>(Rect{}); }},
    {"Container",    []() -> std::unique_ptr<Widget> { return std::make_unique<Container>(); }},
    {"Label",        []() -> std::unique_ptr<Widget> { return std::make_unique<Label>(); }},
    {"ListView",     []() -> std::unique_ptr<Widget> { return std::make_unique<ListView>(Rect{}); }},
    {"Menu",         []() -> std::unique_ptr<Widget> { return std::make_unique<Menu>(); }},
    {"Nav",          []() -> std::unique_ptr<Widget> { return std::make_unique<Nav>(); }},
    {"ModalWindow",  []() -> std::unique_ptr<Widget> { return std::make_unique<ModalWindow>(); }},
    {"ProgressRing", []() -> std::unique_ptr<Widget> { return std::make_unique<ProgressRing>(Rect{}); }},
    {"RadioButton",  []() -> std::unique_ptr<Widget> { return std::make_unique<RadioButton>(); }},
    {"Slider",       []() -> std::unique_ptr<Widget> { return std::make_unique<Slider>(); }},
    {"Switch",       []() -> std::unique_ptr<Widget> { return std::make_unique<Switch>(); }},
    {"Table",        []() -> std::unique_ptr<Widget> { return std::make_unique<Table>(Rect{}); }},
    {"TabView",      []() -> std::unique_ptr<Widget> { return std::make_unique<TabView>(Rect{}); }},
    {"TextInput",    []() -> std::unique_ptr<Widget> { return std::make_unique<TextInput>(); }},
    {"Tooltip",      []() -> std::unique_ptr<Widget> { return std::make_unique<Tooltip>(Rect{}); }},
    {"WebView",      []() -> std::unique_ptr<Widget> { return std::make_unique<WebView>(Rect{}); }},
};

LayoutEntry kLayouts[] = {
    {"Horizontal", []() -> std::unique_ptr<Layout> { return std::make_unique<HorizontalLayout>(); }},
    {"Vertical",   []() -> std::unique_ptr<Layout> { return std::make_unique<VerticalLayout>(); }},
    {"Grid",       []() -> std::unique_ptr<Layout> { return std::make_unique<GridLayout>(); }},
};

WidgetFactory::Creator find_widget(std::string_view name) noexcept {
    for (auto& e : kWidgets)
        if (e.name == name) return e.creator;
    return nullptr;
}

LayoutFactory::Creator find_layout(std::string_view name) noexcept {
    for (auto& e : kLayouts)
        if (e.name == name) return e.creator;
    return nullptr;
}
}

void WidgetFactory::registerWidget(std::string_view name, Creator creator) {
    for (auto& e : kWidgets) {
        if (e.name == name) { e.creator = creator; return; }
    }
}

Result<std::unique_ptr<Widget>>
WidgetFactory::create(std::string_view name) {
    auto creator = find_widget(name);
    if (!creator) {
        return Result<std::unique_ptr<Widget>>::err(
            std::string("WidgetFactory: unknown type '") + std::string(name) + "'");
    }
    return Result<std::unique_ptr<Widget>>(creator());
}

void LayoutFactory::registerLayout(std::string_view name, Creator creator) {
    for (auto& e : kLayouts) {
        if (e.name == name) { e.creator = creator; return; }
    }
}

Result<std::unique_ptr<Layout>>
LayoutFactory::create(std::string_view name) {
    auto creator = find_layout(name);
    if (!creator) {
        return Result<std::unique_ptr<Layout>>::err(
            std::string("LayoutFactory: unknown type '") + std::string(name) + "'");
    }
    return Result<std::unique_ptr<Layout>>(creator());
}

}
