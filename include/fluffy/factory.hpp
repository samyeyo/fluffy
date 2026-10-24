// include/fluffy/factory.hpp
//
// WidgetFactory / LayoutFactory — flat-array type-name registries.

#pragma once
#include <fluffy/error.hpp>
#include <fluffy/layout.hpp>
#include <fluffy/widget.hpp>
#include <memory>
#include <string_view>
namespace fluffy {
class WidgetFactory {
public:
    using Creator = std::unique_ptr<Widget>(*)();
    static void registerWidget(std::string_view name, Creator creator);
    [[nodiscard]] static Result<std::unique_ptr<Widget>>
        create(std::string_view name);
};
class LayoutFactory {
public:
    using Creator = std::unique_ptr<Layout>(*)();
    static void registerLayout(std::string_view name, Creator creator);
    [[nodiscard]] static Result<std::unique_ptr<Layout>>
        create(std::string_view name);
};
}
