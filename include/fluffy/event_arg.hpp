// include/fluffy/event_arg.hpp

// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

// EventArgument — widget pointer + event name + string args for dispatcher.

#pragma once
#include <string>
#include <string_view>
#include <vector>
namespace fluffy { class Widget; }
namespace fluffy {
struct EventArgument {
    Widget*               widget;
    std::string_view      eventName;
    std::vector<std::string> args;
};
}  // namespace fluffy
