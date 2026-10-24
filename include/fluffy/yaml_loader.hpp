// include/fluffy/yaml_loader.hpp

// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

// YAML loader — parseWidgetEntry and loadWidget for Window::load.

#pragma once
#include <fluffy/error.hpp>
#include <fluffy/inline.hpp>
#include <fluffy/widget.hpp>
#include <fluffy/yaml_fwd.hpp>
#include <memory>
#include <string_view>
namespace fluffy {
[[nodiscard]] Result<std::unique_ptr<Widget>>
    loadWidget(const YamlNode& node);
[[nodiscard]] Result<std::unique_ptr<Widget>>
    parseWidgetEntry(const std::string& type, const YamlNode& body);

// ––––––––[ parseInlineYaml ] parse an `inline:` run list into Inline content.
[[nodiscard]] Result<Inline> parseInlineYaml(const YamlNode& node);
}  

