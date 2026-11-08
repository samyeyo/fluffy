// src/yaml_loader.cpp

#include <fluffy/yaml_loader.hpp>
#include <fluffy/button.hpp>
#include <fluffy/container.hpp>
#include <fluffy/factory.hpp>
#include <fluffy/icons.hpp>
#include <fluffy/label.hpp>
#include <fluffy/layout.hpp>
#include <fluffy/yaml_fwd.hpp>
#include <cstdlib>

namespace fluffy {
namespace {
bool isEventKey(const std::string& k) noexcept {
    return k.size() >= 3 && k[0] == 'o' && k[1] == 'n' && (k[2] >= 'A' && k[2] <= 'Z');
}
void applyScalarProperty(const std::string& /*type*/, Widget& w,
                         const std::string& key,
                         const YamlNode& value) {
    w.setProperty(key, value);
}

bool parseBool(const std::string& raw, bool& out) {
    if (raw == "true" || raw == "True" || raw == "TRUE" || raw == "1") {
        out = true;
        return true;
    }
    if (raw == "false" || raw == "False" || raw == "FALSE" || raw == "0") {
        out = false;
        return true;
    }
    return false;
}
}

Result<Inline> parseInlineYaml(const YamlNode& node) {
    if (!node.IsMap()) {
        return Result<Inline>::err(
            "yaml_loader: inline must be a list of runs");
    }

    Inline content;
    std::optional<Color> fg;
    float size = 0.0f;
    bool bold = false;
    bool italic = false;
    bool has_run = false;

    auto apply_style = [&] {
        content << Style{fg, size, bold, italic};
    };

    for (const auto& run : node) {
        const std::string key = run.key();
        if (!run.IsScalar()) {
            return Result<Inline>::err(
                "yaml_loader: inline '" + key + "' must be a scalar");
        }

        if (key == "text") {
            const std::string text = run.asString();
            if (!text.empty()) {
                content << text;
                has_run = true;
            }
        } else if (key == "icon") {
            const std::string name = run.asString();
            const SvgIconRef* icon = Icons::find(name);
            if (!icon || !icon->svg_data) {
                return Result<Inline>::err(
                    "yaml_loader: unknown icon '" + name + "'");
            }
            content << *icon;
            has_run = true;
        } else if (key == "color") {
            const std::string raw = run.asString();
            auto color = Color::fromString(raw);
            if (!color) {
                return Result<Inline>::err(
                    "yaml_loader: invalid inline color '" + raw + "'");
            }
            fg = *color;
            apply_style();
        } else if (key == "size") {
            const std::string raw = run.asString();
            char* end = nullptr;
            const float value = std::strtof(raw.c_str(), &end);
            if (raw.empty() || *end != '\0' || !(value >= 0.0f)) {
                return Result<Inline>::err(
                    "yaml_loader: invalid inline size '" + raw + "'");
            }
            size = value;
            apply_style();
        } else if (key == "bold" || key == "italic") {
            const std::string raw = run.asString();
            bool value = false;
            if (!parseBool(raw, value)) {
                return Result<Inline>::err(
                    "yaml_loader: invalid inline " + key + " '" + raw + "'");
            }
            if (key == "bold") bold = value;
            else               italic = value;
            apply_style();
        } else if (key == "reset") {
            const std::string raw = run.asString();
            bool value = false;
            if (!parseBool(raw, value)) {
                return Result<Inline>::err(
                    "yaml_loader: invalid inline reset '" + raw + "'");
            }
            if (value) {
                fg.reset();
                size = 0.0f;
                bold = false;
                italic = false;
                apply_style();
            }
        } else {
            return Result<Inline>::err(
                "yaml_loader: unknown inline run '" + key + "'");
        }
    }

    if (!has_run) {
        return Result<Inline>::err(
            "yaml_loader: inline must contain at least one text or icon run");
    }
    return content;
}

Result<std::unique_ptr<Widget>>
parseWidgetEntry(const std::string& type, const YamlNode& body) {
    auto wres = WidgetFactory::create(type);
    if (!wres) {
        return Result<std::unique_ptr<Widget>>::err(wres.error());
    }
    auto widget = std::move(wres).value();

    if ((type == "Label" || type == "Button") && body.IsMap()) {
        bool has_text = false;
        bool has_inline = false;
        for (const auto& child : body) {
            const std::string key = child.key();
            if (key == "text" && child.IsScalar()) has_text = true;
            if (key == "inline" && child.IsMap()) has_inline = true;
        }
        if (has_text && has_inline) {
            return Result<std::unique_ptr<Widget>>::err(
                "yaml_loader: " + type + " cannot use both 'text' and 'inline'");
        }
    }

    if (type == "Container") {
        if (!body.IsMap() && !body.IsNull()) {
            return Result<std::unique_ptr<Widget>>::err(
                "yaml_loader: Container body must be a map of properties + children");
        }
        auto* c = static_cast<Container*>(widget.get());

        // First pass: scalar properties
        for (const auto& child : body) {
            const std::string key = child.key();
            if (child.IsScalar()) {
                applyScalarProperty(type, *widget, key, child);
            }
        }

        // Second pass: map children (layout + widget children)
        bool layout_done = false;
        for (const auto& child : body) {
            const std::string key = child.key();
            if (!child.IsMap()) continue;

            if (!layout_done) {
                auto lres = LayoutFactory::create(key);
                if (lres) {
                    auto layout_owned = std::move(lres).value();
                    for (const auto& lp : child) {
                        const std::string lk = lp.key();
                        layout_owned->setProperty(lk, lp);
                    }
                    c->setLayout(std::move(layout_owned));
                    layout_done = true;
                    continue;
                }
                // Not a layout name: allow a layout-less container when the
                // key names a real widget (constraints / manual bounds drive
                // placement). Otherwise keep the helpful error for typos.
                auto wprobe = WidgetFactory::create(key);
                if (!wprobe) {
                    return Result<std::unique_ptr<Widget>>::err(
                        "yaml_loader: Container's first map child must "
                        "be a layout (got '" + key + "')");
                }
                layout_done = true; // skip layout parsing once; parse as widget below
            }

            auto child_res = parseWidgetEntry(key, child);
            if (child_res) {
                c->addChild(std::move(child_res).value());
            }
        }

        if (!layout_done) {
        }
        return Result<std::unique_ptr<Widget>>(std::move(widget));
    }

    if (body.IsMap()) {
        for (const auto& child : body) {
            const std::string key = child.key();
            if (key == "inline" && (type == "Label" || type == "Button")) {
                if (!child.IsMap()) {
                    return Result<std::unique_ptr<Widget>>::err(
                        "yaml_loader: " + type + " 'inline' must be a list of runs");
                }
                auto inline_res = parseInlineYaml(child);
                if (!inline_res) {
                    return Result<std::unique_ptr<Widget>>::err(
                        inline_res.error());
                }
                if (type == "Label") {
                    static_cast<Label*>(widget.get())->setInline(
                        std::move(inline_res).value());
                } else {
                    static_cast<Button*>(widget.get())->setInline(
                        std::move(inline_res).value());
                }
                continue;
            }
            if (isEventKey(key)) {
                if (child.IsScalar()) {
                    widget->setEvent(key, child.asString());
                }
            } else if (child.IsScalar()) {
                applyScalarProperty(type, *widget, key, child);
            } else if (child.IsMap()) {
                // Map-valued properties (e.g. `constraints:`) go straight
                // to the widget; unknown map keys are ignored downstream.
                widget->setProperty(key, child);
            }
        }
    }

    return Result<std::unique_ptr<Widget>>(std::move(widget));
}

Result<std::unique_ptr<Widget>>
loadWidget(const YamlNode& node) {
    if (!node.IsMap() || node.size() != 1) {
        return Result<std::unique_ptr<Widget>>::err(
            "yaml_loader: every widget node must be a map with EXACTLY one "
            "key (the type), e.g. `Button: { ... }`");
    }
    auto it = node.begin();
    YamlNode child = *it;
    return parseWidgetEntry(child.key(), child);
}
}
