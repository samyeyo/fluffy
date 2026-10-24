// include/fluffy/event.hpp

// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

// Event — std::variant event types and EventKind dispatch helper.

#pragma once
#include <variant>
namespace fluffy {

// ––––––––[ MouseButton ] platform-independent mouse buttons
enum class MouseButton { Left, Right, Middle, Button4, Button5, Unknown };

struct MouseMoveEvent     { float x; float y; };
struct MouseDownEvent     { float x; float y; MouseButton button; };
struct MouseUpEvent       { float x; float y; MouseButton button; };
struct ResizeEvent        { int width; int height; };
struct KeyDownEvent       { int key; int scancode; int mods; };
struct CharEvent          { unsigned int codepoint; };
struct MouseScrollEvent   { float dx; float dy; };
struct CloseEvent         {};
using Event =
    std::variant<MouseMoveEvent,
                 MouseDownEvent,
                 MouseUpEvent,
                 ResizeEvent,
                 KeyDownEvent,
                 CharEvent,
                 MouseScrollEvent,
                 CloseEvent>;
enum class EventKind {
    MouseMove = 0, MouseDown = 1, MouseUp = 2,
    Resize    = 3, KeyDown  = 4, Char    = 5,
    MouseScroll = 6, Close = 7,
};
constexpr EventKind kind_of(const Event& e) noexcept {
    return static_cast<EventKind>(e.index());
}
}  

