// src/dispatcher.cpp
//
// EventDispatcher — registerHandler + invoke (flat vector).

#include <fluffy/dispatcher.hpp>
#include <fluffy/geom.hpp>
#include <fluffy/widget.hpp>
namespace fluffy {
void EventDispatcher::registerHandler(std::string_view name, Handler handler) {
    const std::string key{name};
    for (auto& [k, h] : handlers_) {
        if (k == key) {
            h = std::move(handler);
            return;
        }
    }
    handlers_.emplace_back(key, std::move(handler));
}

void EventDispatcher::invoke(const EventArgument& arg) {
    const std::string key{arg.eventName};
    for (auto& [k, h] : handlers_) {
        if (k == key) {
            h(arg);
            return;
        }
    }
}
}
