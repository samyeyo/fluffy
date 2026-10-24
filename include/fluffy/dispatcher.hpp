// include/fluffy/dispatcher.hpp
//
// EventDispatcher — per-Window handler registration and invocation.

#pragma once
#include <fluffy/event_arg.hpp>
#include <functional>
#include <string>
#include <string_view>
#include <vector>
namespace fluffy {
class EventDispatcher {
public:
    using Handler = std::function<void(const EventArgument&)>;
    void registerHandler(std::string_view name, Handler handler);
    void invoke(const EventArgument& arg);
private:
    std::vector<std::pair<std::string, Handler>> handlers_;
};
}
