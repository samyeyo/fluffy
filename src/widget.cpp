// src/widget.cpp

// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

// Widget — default setProperty / setEvent / triggerEvent implementations.

#include <fluffy/widget.hpp>
#include <fluffy/constraint.hpp>
#include <fluffy/event_arg.hpp>
#include <fluffy/window.hpp>
#include <fluffy/yaml_fwd.hpp>
namespace fluffy {
float Widget::naturalWidth(const Theme&, const Window&) const { return 0.0f; }
float Widget::naturalHeight(const Theme&, const Window&) const { return 0.0f; }
Widget::~Widget() {
    clearConstraints(*this);
}
void Widget::setProperty(std::string_view name, const YamlNode& value) {
    if (name == "x") {
        auto b = bounds_; b.x = value.asFloat(); setBounds(b); return;
    }
    if (name == "y") {
        auto b = bounds_; b.y = value.asFloat(); setBounds(b); return;
    }
    if (name == "w" || name == "width") {
        auto b = bounds_; b.w = value.asFloat(); setBounds(b); return;
    }
    if (name == "h" || name == "height") {
        auto b = bounds_; b.h = value.asFloat(); setBounds(b); return;
    }
    if (name == "visible") { setVisible(value.asBool());   return; }
    if (name == "enabled") { setEnabled(value.asBool());   return; }
    if (name == "font")    { setFont(value.asString()); return; }
    if (name == "autoSize" || name == "auto_size") {
        const std::string raw = value.asString();
        if      (raw == "disabled" || raw == "none")   setAutoSize(AutoSize::Disabled);
        else if (raw == "width"   || raw == "w")       setAutoSize(AutoSize::Width);
        else if (raw == "height"  || raw == "h")       setAutoSize(AutoSize::Height);
        else                                              setAutoSize(AutoSize::Both);
        return;
    }
    if (name == "font_size" || name == "text_size") {
        setTextSize(value.asFloat());
        return;
    }
    if (name == "userdata") {
        userdata_ = value.asString();
        return;
    }
    if (name == "accent") {
        auto c = Color::fromString(value.asString());
        if (c) accent_override_ = *c;
        return;
    }
    if (name == "constraints") {
        ConstraintSet cs;
        if (parseConstraintsNode(value, cs)) {
            constrain(*this) = cs;
        }
        return;
    }
}
void Widget::setEvent(std::string_view name, std::string_view handlerName) {
    for (auto& [k, h] : event_bindings_) {
        if (k == name) { h = handlerName; return; }
    }
    event_bindings_.emplace_back(std::string{name}, std::string{handlerName});
}
void Widget::triggerEvent(
    std::string_view eventName,
    const std::vector<std::string>& args) const {
    for (const auto& [k, h] : event_bindings_) {
        if (k == eventName) {
            if (!window_) {
                fprintf(stderr, "[fluffy] triggerEvent: '%s' fired but widget has no "
                        "owning Window (bounds=%.0f,%.0f,%.0f,%.0f).\n",
                        std::string(eventName).c_str(),
                        bounds_.x, bounds_.y, bounds_.w, bounds_.h);
                return;
            }
            EventArgument arg{
                const_cast<Widget*>(this),
                h,
                args,
            };
            window_->dispatcher().invoke(arg);
            return;
        }
    }
}
}  

