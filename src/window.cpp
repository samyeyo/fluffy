// src/window.cpp

// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

// Window — create, run, renderFrame, showDialog, font registry, GLFW callbacks.

#include <fluffy/window.hpp>
#include <fluffy/button.hpp>     
#include <fluffy/container.hpp>  
#include <fluffy/error.hpp>
#include <fluffy/font.hpp>
#include <fluffy/label.hpp>      
#include <fluffy/layout.hpp>     
#include <fluffy/theme.hpp>
#include <fluffy/tooltip.hpp>
#include <fluffy/widget.hpp>
#include <fluffy/yaml_loader.hpp> 
#if defined(__EMSCRIPTEN__) || defined(FLUFFY_EMSCRIPTEN)
#include <GLES2/gl2.h>
#include <GLES3/gl3.h>
#include <GLES2/gl2ext.h>
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#ifndef GL_RGBA8
#define GL_RGBA8 GL_RGBA
#endif
#ifndef GL_READ_FRAMEBUFFER
#define GL_READ_FRAMEBUFFER GL_FRAMEBUFFER
#endif
#else
#include <gl3w.h>
#endif
#include <nanovg.h>
#include <nanovg_gl.h>  
#include <GLFW/glfw3.h>
#include <format>
#ifndef __EMSCRIPTEN__
#if defined(__APPLE__)
#  define GLFW_EXPOSE_NATIVE_COCOA
#elif defined(__linux__)
#  define GLFW_EXPOSE_NATIVE_X11
#elif defined(_WIN32)
#  define GLFW_EXPOSE_NATIVE_WIN32
#endif
#include <GLFW/glfw3native.h>
#if defined(__APPLE__)
#  include <objc/runtime.h>
#  include <objc/message.h>
#elif defined(__linux__)
#  include <X11/Xlib.h>
#  include <X11/Xatom.h>
#  include <X11/Xutil.h>
// Xlib defines macros that collide with Fluffy's enum names.
#  undef None
#  undef Button4
#  undef Button5
#  undef CursorShape
#endif
#endif
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <fluffy/yaml_fwd.hpp>
#include <fluffy/application.hpp>

#ifndef FLUFFY_EMSCRIPTEN
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>
#endif
#include <fluffy/tooltip.hpp>

namespace fluffy {
struct Window::Impl {
    GLFWwindow*      glfw_window = nullptr;
    NVGcontext*      vg          = nullptr;
    int   logical_w = 0,  logical_h = 0;
    int   fb_w      = 0,  fb_h      = 0;
    float scale_x   = 1.0f, scale_y = 1.0f;
    float coord_scale_x = 1.0f, coord_scale_y = 1.0f;
    float forced_scale_x = 0.0f, forced_scale_y = 0.0f;
    bool  decorated = true;
    bool  init_failed_ = false;
    int   design_w        = 0;
    int   design_h        = 0;
    float design_px_ratio = 1.0f;
    GLuint fbo         = 0;
    GLuint fbo_color   = 0;
    GLuint fbo_stencil = 0;
    int    fbo_w       = 0;
    int    fbo_h       = 0;
    mutable std::unordered_set<std::string>                        loaded_faces_;
    mutable std::unordered_map<std::string, std::vector<unsigned char>> mem_fonts_;
    GLFWcursor* cursors[6] = {};
    CursorShape currentCursorShape = CursorShape::Arrow;
    Impl() {
        // Ensure GLFW is initialized via lazy Application singleton.
        Application::get();
        // Application handles glfwInit; if it failed, mark init_failed.
        // Check via tryGet and glfwInited_? For now assume success.
        // If Application failed to init GLFW, we could check but leave false.
    }
    ~Impl() {
        if (fbo)         { glDeleteFramebuffers  (1, &fbo);         fbo         = 0; }
        if (fbo_color)   { glDeleteTextures      (1, &fbo_color);   fbo_color   = 0; }
        if (fbo_stencil) { glDeleteRenderbuffers (1, &fbo_stencil); fbo_stencil = 0; }
    #ifdef FLUFFY_EMSCRIPTEN
    if (vg) { nvgDeleteGLES2(vg); vg = nullptr; }
#else
    if (vg) { nvgDeleteGL3(vg); vg = nullptr; }
#endif
        for (GLFWcursor*& cursor : cursors) {
            if (cursor) { glfwDestroyCursor(cursor); cursor = nullptr; }
        }
        if (glfw_window) { glfwDestroyWindow(glfw_window); glfw_window = nullptr; }
    }
};
namespace {
Window* from_glfw(GLFWwindow* w) noexcept {
    return static_cast<Window*>(glfwGetWindowUserPointer(w));
}

// ––––––––[ collectFocusable ] depth-first list of visible, enabled, focusable widgets
void collectFocusable(Widget* w, std::vector<Widget*>& out) {
    if (!w || !w->visible() || !w->enabled()) return;
    if (w->isFocusable()) out.push_back(w);
    w->forEachChild([&out](Widget* c) { collectFocusable(c, out); });
}

// ––––––––[ glfwCursorFor ] maps a CursorShape to its GLFW standard cursor id
int glfwCursorFor(CursorShape shape) noexcept {
    switch (shape) {
    case CursorShape::IBeam:     return GLFW_IBEAM_CURSOR;
    case CursorShape::Hand:      return GLFW_HAND_CURSOR;
    case CursorShape::Crosshair: return GLFW_CROSSHAIR_CURSOR;
    case CursorShape::HResize:   return GLFW_HRESIZE_CURSOR;
    case CursorShape::VResize:   return GLFW_VRESIZE_CURSOR;
    case CursorShape::Arrow:     break;
    }
    return GLFW_ARROW_CURSOR;
}

// ––––––––[ mouseButtonFromGlfw ] maps a GLFW mouse button id to MouseButton
MouseButton mouseButtonFromGlfw(int button) noexcept {
    switch (button) {
    case GLFW_MOUSE_BUTTON_LEFT:   return MouseButton::Left;
    case GLFW_MOUSE_BUTTON_RIGHT:  return MouseButton::Right;
    case GLFW_MOUSE_BUTTON_MIDDLE: return MouseButton::Middle;
    case GLFW_MOUSE_BUTTON_4:      return MouseButton::Button4;
    case GLFW_MOUSE_BUTTON_5:      return MouseButton::Button5;
    default:                       return MouseButton::Unknown;
    }
}

#if defined(__linux__)
// Motif window-manager hint bits (not provided by the X11 headers).
constexpr unsigned long kMwmHintsFunctions   = 1UL << 0;
constexpr unsigned long kMwmHintsDecorations = 1UL << 1;
constexpr unsigned long kMwmFuncResize       = 1UL << 1;
constexpr unsigned long kMwmFuncMove         = 1UL << 2;
constexpr unsigned long kMwmFuncMinimize     = 1UL << 3;
constexpr unsigned long kMwmFuncMaximize     = 1UL << 4;
constexpr unsigned long kMwmFuncClose        = 1UL << 5;
constexpr unsigned long kMwmDecorAll         = 1UL << 0;

// ––––––––[ x11_set_window_type ] tags the window for the window manager.
void x11_set_window_type(GLFWwindow* win, const char* type_name) {
    if (!win || glfwGetPlatform() != GLFW_PLATFORM_X11) return;
    Display* display = glfwGetX11Display();
    if (!display) return;
    ::Window x11_win = glfwGetX11Window(win);
    if (!x11_win) return;
    Atom wm_type = XInternAtom(display, "_NET_WM_WINDOW_TYPE", False);
    Atom target  = XInternAtom(display, type_name, False);
    XChangeProperty(display, x11_win, wm_type, XA_ATOM, 32,
                    PropModeReplace,
                    reinterpret_cast<unsigned char*>(&target), 1);
    XFlush(display);
}

// ––––––––[ x11_set_decorations ] toggles WM chrome and button availability.
void x11_set_decorations(GLFWwindow* win, bool decorated,
                         WindowButtons buttons) {
    if (!win || glfwGetPlatform() != GLFW_PLATFORM_X11) return;
    Display* display = glfwGetX11Display();
    if (!display) return;
    ::Window x11_win = glfwGetX11Window(win);
    if (!x11_win) return;

    struct MotifHints {
        unsigned long flags;
        unsigned long functions;
        unsigned long decorations;
        long          input_mode;
        unsigned long status;
    } hints{};
    hints.flags = kMwmHintsFunctions | kMwmHintsDecorations;
    if (!decorated) {
        hints.functions   = 0;
        hints.decorations = 0;
    } else {
        const auto has = [buttons](WindowButtons b) {
            return (buttons & b) != WindowButtons::None;
        };
        hints.functions = kMwmFuncResize | kMwmFuncMove;
        if (has(WindowButtons::Close))
            hints.functions |= kMwmFuncClose;
        if (has(WindowButtons::Minimize))
            hints.functions |= kMwmFuncMinimize;
        if (has(WindowButtons::Maximize))
            hints.functions |= kMwmFuncMaximize;
        hints.decorations = kMwmDecorAll;
    }

    Atom motif = XInternAtom(display, "_MOTIF_WM_HINTS", False);
    XChangeProperty(display, x11_win, motif, motif, 32, PropModeReplace,
                    reinterpret_cast<unsigned char*>(&hints),
                    static_cast<int>(sizeof(hints) / sizeof(long)));
    XFlush(display);
}

// ––––––––[ x11_scale_override ] reads an explicit user scaling preference.
float x11_scale_override() {
    if (glfwGetPlatform() != GLFW_PLATFORM_X11) return 0.0f;
    if (const char* s = std::getenv("FLUFFY_SCALE")) {
        char* end = nullptr;
        const float v = std::strtof(s, &end);
        if (end != s && v >= 1.0f && v <= 4.0f) return v;
    }
    if (const char* gdk = std::getenv("GDK_SCALE")) {
        if (*gdk) {
            const int v = std::atoi(gdk);
            if (v >= 1 && v <= 4) return static_cast<float>(v);
        }
    }
    if (const char* qt = std::getenv("QT_SCALE_FACTOR")) {
        if (*qt) {
            char* end = nullptr;
            const float v = std::strtof(qt, &end);
            if (end != qt && v >= 1.0f && v <= 4.0f) return v;
        }
    }
    return 0.0f;
}

// ––––––––[ x11_content_scale_fallback ] infers a scale GLFW may not report.
float x11_content_scale_fallback() {
    if (glfwGetPlatform() != GLFW_PLATFORM_X11) return 0.0f;
    // Last resort: infer a deliberately high DPI from the primary monitor.
    if (GLFWmonitor* monitor = glfwGetPrimaryMonitor()) {
        int mm_w = 0, mm_h = 0;
        glfwGetMonitorPhysicalSize(monitor, &mm_w, &mm_h);
        if (const GLFWvidmode* mode = glfwGetVideoMode(monitor)) {
            if (mm_w > 0 && mode->width > 0) {
                const float dpi = static_cast<float>(mode->width) * 25.4f
                                / static_cast<float>(mm_w);
                float scale = dpi / 96.0f;
                if (scale >= 1.5f) {
                    scale = std::round(scale * 4.0f) / 4.0f;
                    return std::min(scale, 4.0f);
                }
            }
        }
    }
    return 0.0f;
}
#endif
}  

// ––––––––[ s_cursor_pos_cb ] dispatch MouseMove directly to topmost widget.
void Window::s_cursor_pos_cb(GLFWwindow* w, double x, double y) {
    if (auto* win = from_glfw(w)) {
        if (!win->impl_) return;
        win->cursor_x_ = static_cast<float>(x) * win->impl_->coord_scale_x;
        win->cursor_y_ = static_cast<float>(y) * win->impl_->coord_scale_y;
        const Event e = MouseMoveEvent{ win->cursor_x_, win->cursor_y_ };
        if (win->eventHandler_ &&
            win->eventHandler_(e, *win) == EventResult::Consumed)
            return;
        for (auto it = win->widgets_.rbegin(); it != win->widgets_.rend(); ++it) {
            auto& wgt = *it;
            if (wgt->visible() && wgt->enabled() &&
                wgt->handleEvent(e, *win) == EventResult::Consumed)
                break;
        }
        flushDeferredPopupClose(win);
    }
}

// ––––––––[ s_mouse_btn_cb ] dispatch MouseDown/MouseUp directly + focus logic + popup close.
void Window::s_mouse_btn_cb(GLFWwindow* w, int button, int action, int /*mods*/) {
    auto* win = from_glfw(w);
    if (!win || !win->impl_) return;

    // Close popup on any MouseDown outside the popup
    if (action == GLFW_PRESS && win->popupPtr_) {
        const bool inside = win->popupPtr_->visible() &&
            win->popupPtr_->containsPoint(win->cursor_x_, win->cursor_y_);
        if (!inside) {
            win->closePopup();
        }
    }

    Event e;
    if (action == GLFW_PRESS)
        e = MouseDownEvent{ win->cursor_x_, win->cursor_y_,
                            mouseButtonFromGlfw(button) };
    else if (action == GLFW_RELEASE)
        e = MouseUpEvent{ win->cursor_x_, win->cursor_y_,
                          mouseButtonFromGlfw(button) };
    else
        return;
    if (win->eventHandler_ &&
        win->eventHandler_(e, *win) == EventResult::Consumed)
        return;
    bool consumed = false;
    for (auto it = win->widgets_.rbegin(); it != win->widgets_.rend(); ++it) {
        auto& wgt = *it;
        if (wgt->visible() && wgt->enabled() &&
            wgt->handleEvent(e, *win) == EventResult::Consumed) {
            consumed = true;
            if (action == GLFW_PRESS) {
                if (wgt->isFocusable()) {
                    if (win->focusWidget_ && win->focusWidget_ != wgt.get())
                        win->focusWidget_->setFocused(false);
                    win->focusWidget_ = wgt.get();
                    win->focusWidget_->setFocused(true);
                }
            }
            break;
        }
    }
    if (!consumed && action == GLFW_PRESS) {
        if (win->focusWidget_) {
            win->focusWidget_->setFocused(false);
            win->focusWidget_ = nullptr;
        }
    }
    flushDeferredPopupClose(win);
}

// ––––––––[ s_framebuffer_size_cb ] resize design space, FBO, root widgets, then dispatch.
void Window::s_framebuffer_size_cb(GLFWwindow* w, int /*w2*/, int /*h2*/) {
    auto* win = from_glfw(w);
    if (!win || !win->impl_) return;
    win->syncSize();
    win->resizeFbo();
    // Close popup on window resize (repositioning would be wrong).
    win->closePopup();
    // Resize root Containers to match new logical window size.
    // Non-Container root widgets (absolute-positioned) keep their manual bounds.
    const int dw = win->impl_->design_w;
    const int dh = win->impl_->design_h;
    for (auto& wgt : win->widgets_) {
        if (wgt->fillsParent())
            wgt->fillToWindow(float(dw), float(dh));
    }
    const Event e = ResizeEvent{ dw, dh };
    for (auto& wgt : win->widgets_) wgt->onResize(ResizeEvent{ dw, dh });
    bool consumed = false;
    if (win->eventHandler_ &&
        win->eventHandler_(e, *win) == EventResult::Consumed)
        consumed = true;
    if (!consumed) {
        for (auto it = win->widgets_.rbegin(); it != win->widgets_.rend(); ++it) {
            if ((*it)->visible() && (*it)->enabled() &&
                (*it)->handleEvent(e, *win) == EventResult::Consumed)
                break;
        }
    }
    if (win->onResize_)
        win->onResize_(dw, dh);
    win->renderFrame();
}

// ––––––––[ s_key_cb ] Tab focus traversal + dispatch KeyDown directly.
void Window::s_key_cb(GLFWwindow* w, int key, int scancode, int action, int mods) {
    auto* win = from_glfw(w);
    if (!win || (action != GLFW_PRESS && action != GLFW_REPEAT)) return;
    const Event e = KeyDownEvent{ key, scancode, mods };
    if (win->eventHandler_ &&
        win->eventHandler_(e, *win) == EventResult::Consumed)
        return;
    // Escape closes popup
    if (key == GLFW_KEY_ESCAPE && win->popupPtr_) {
        win->closePopup();
        return;
    }

    const KeyDownEvent ke{ key, scancode, mods };
    bool consumed = false;
    for (auto it = win->widgets_.rbegin(); it != win->widgets_.rend(); ++it) {
        auto& wgt = *it;
        if (wgt->visible() && wgt->enabled() &&
            wgt->handleEvent(ke, *win) == EventResult::Consumed) {
            consumed = true;
            break;
        }
    }
    // Unhandled Tab drives global focus traversal (Shift+Tab walks backwards).
    if (!consumed && key == GLFW_KEY_TAB && action == GLFW_PRESS) {
        win->focusStep((mods & GLFW_MOD_SHIFT) ? -1 : +1);
    }
    flushDeferredPopupClose(win);
}

// ––––––––[ s_char_cb ] dispatch CharEvent directly.
void Window::s_char_cb(GLFWwindow* w, unsigned int codepoint) {
    if (auto* win = from_glfw(w)) {
        const Event e = CharEvent{ codepoint };
        if (win->eventHandler_ &&
            win->eventHandler_(e, *win) == EventResult::Consumed)
            return;
        for (auto it = win->widgets_.rbegin(); it != win->widgets_.rend(); ++it) {
            auto& wgt = *it;
            if (wgt->visible() && wgt->enabled() &&
                wgt->handleEvent(e, *win) == EventResult::Consumed)
                break;
        }
        flushDeferredPopupClose(win);
    }
}

// ––––––––[ s_scroll_cb ] dispatch MouseScroll to topmost widget.
void Window::s_scroll_cb(GLFWwindow* w, double x, double y) {
    if (auto* win = from_glfw(w)) {
        const Event e = MouseScrollEvent{
            static_cast<float>(x), static_cast<float>(y)
        };
        if (win->eventHandler_ &&
            win->eventHandler_(e, *win) == EventResult::Consumed)
            return;
        for (auto it = win->widgets_.rbegin(); it != win->widgets_.rend(); ++it) {
            auto& wgt = *it;
            if (wgt->visible() && wgt->enabled() &&
                wgt->handleEvent(e, *win) == EventResult::Consumed)
                break;
        }
        flushDeferredPopupClose(win);
    }
}

// ––––––––[ s_close_cb ] set should_close flag.
void Window::s_close_cb(GLFWwindow* w) {
    if (auto* win = from_glfw(w)) {
        const Event e = CloseEvent{};
        if (win->eventHandler_ &&
            win->eventHandler_(e, *win) == EventResult::Consumed)
            return;
        win->should_close_ = true;
        glfwSetWindowShouldClose(w, GLFW_TRUE);
    }
}

Window::Window(Window&& other) noexcept
    : impl_(std::move(other.impl_))
    , dispatcher_(std::move(other.dispatcher_))
    , theme_(std::move(other.theme_))
    , widgets_(std::move(other.widgets_))
    , onResize_(std::move(other.onResize_))
    , eventHandler_(std::move(other.eventHandler_))
    , should_close_(other.should_close_)
    , cursor_x_(other.cursor_x_)
    , cursor_y_(other.cursor_y_)
    , pending_events_(std::move(other.pending_events_))
    , last_time_(other.last_time_)
    , enabledButtons_(other.enabledButtons_)
    , root_container_(other.root_container_)
    , focusWidget_(other.focusWidget_)
    , popupPtr_(other.popupPtr_)
    , popupPendingClose_(other.popupPendingClose_)
    , prevPopups_(std::move(other.prevPopups_))
    , tooltip_hover_widget_(other.tooltip_hover_widget_)
    , tooltip_hover_start_(other.tooltip_hover_start_) {
    other.root_container_ = nullptr;
    other.focusWidget_ = nullptr;
    other.popupPtr_ = nullptr;
    other.tooltip_hover_widget_ = nullptr;
    if (impl_) {
        // Re-register at new address after move
        if (auto* app = Application::tryGet()) app->unregisterWindow(&other);
        Application::get().registerWindow(this);
        // Update window_ pointer for all widgets
        for (auto& wgt : widgets_) {
            if (wgt) wgt->setWindow(this);
        }
        // Rebind GLFW user pointer
        glfwSetWindowUserPointer(impl_->glfw_window, this);
    }
}

Window& Window::operator=(Window&& other) noexcept {
    if (this == &other) return *this;
    if (impl_) {
        if (auto* app = Application::tryGet()) app->unregisterWindow(this);
    }
    impl_ = std::move(other.impl_);
    dispatcher_ = std::move(other.dispatcher_);
    theme_ = std::move(other.theme_);
    widgets_ = std::move(other.widgets_);
    onResize_ = std::move(other.onResize_);
    eventHandler_ = std::move(other.eventHandler_);
    should_close_ = other.should_close_;
    cursor_x_ = other.cursor_x_;
    cursor_y_ = other.cursor_y_;
    pending_events_ = std::move(other.pending_events_);
    last_time_ = other.last_time_;
    enabledButtons_ = other.enabledButtons_;
    root_container_ = other.root_container_;
    focusWidget_ = other.focusWidget_;
    popupPtr_ = other.popupPtr_;
    popupPendingClose_ = other.popupPendingClose_;
    prevPopups_ = std::move(other.prevPopups_);
    tooltip_hover_widget_ = other.tooltip_hover_widget_;
    tooltip_hover_start_ = other.tooltip_hover_start_;
    other.root_container_ = nullptr;
    other.focusWidget_ = nullptr;
    other.popupPtr_ = nullptr;
    other.tooltip_hover_widget_ = nullptr;
    if (impl_) {
        if (auto* app = Application::tryGet()) app->unregisterWindow(&other);
        Application::get().registerWindow(this);
        for (auto& wgt : widgets_) {
            if (wgt) wgt->setWindow(this);
        }
        glfwSetWindowUserPointer(impl_->glfw_window, this);
    }
    return *this;
}

Window::~Window() {
    if (impl_) {
        if (auto* app = Application::tryGet()) app->unregisterWindow(this);
    }
}

// ––––––––[ create ] GLFW window, OpenGL context, FBO, NanoVG, system fonts.
Result<Window> Window::create(const Config& cfg) {
    Window win;
    win.theme_ = cfg.theme;
    win.impl_ = std::make_unique<Impl>();
    if (win.impl_->init_failed_) {
        return Result<Window>::err("glfwInit failed");
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_STENCIL_BITS, 8);
    glfwWindowHint(GLFW_RESIZABLE, cfg.resizable ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_VISIBLE, cfg.visible ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_DECORATED, cfg.decorated ? GLFW_TRUE : GLFW_FALSE);
#if defined(GLFW_SCALE_TO_MONITOR) && !defined(__APPLE__) && \
    !defined(FLUFFY_EMSCRIPTEN) && !defined(__EMSCRIPTEN__)
    glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);
#endif
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    GLFWwindow* glfw = glfwCreateWindow(cfg.width, cfg.height,
                                         cfg.title.c_str(),
                                         /*monitor=*/nullptr,
                                         /*share=  */nullptr);
    if (!glfw) {
        return Result<Window>::err("glfwCreateWindow failed");
    }
    glfwMakeContextCurrent(glfw);
    if (cfg.vsync) glfwSwapInterval(1);
#if !defined(FLUFFY_EMSCRIPTEN) && !defined(__EMSCRIPTEN__)
    if (gl3wInit2(glfwGetProcAddress) != 0) {
        return Result<Window>::err("gl3wInit2 failed");
    }
#endif
    win.impl_->glfw_window = glfw;
    win.impl_->decorated   = cfg.decorated;
#if defined(__linux__)
    if (cfg.decorated)
        x11_set_window_type(glfw, "_NET_WM_WINDOW_TYPE_NORMAL");
    // GLFW derives the X11 scale from Xft.dpi; cover setups that only expose
    // scaling through the toolkit environment or monitor DPI.
    {
        float gsx = 1.0f, gsy = 1.0f;
        glfwGetWindowContentScale(glfw, &gsx, &gsy);
        float target = x11_scale_override();
        if (target <= 0.0f) {
            const float fallback = x11_content_scale_fallback();
            if (fallback > gsx + 0.01f || fallback > gsy + 0.01f)
                target = fallback;
        }
        if (target > 0.0f) {
            win.impl_->forced_scale_x = target;
            win.impl_->forced_scale_y = target;
            glfwSetWindowSize(glfw,
                static_cast<int>(std::lround(cfg.width  * target)),
                static_cast<int>(std::lround(cfg.height * target)));
        }
    }
    win.applyWindowButtons();
#endif
    win.impl_->design_w    = cfg.width;
    win.impl_->design_h    = cfg.height;
    win.syncSize();
    if (cfg.min_width > 0 || cfg.min_height > 0) {
        const float invx = win.impl_->coord_scale_x > 0.0f
            ? 1.0f / win.impl_->coord_scale_x : 1.0f;
        const float invy = win.impl_->coord_scale_y > 0.0f
            ? 1.0f / win.impl_->coord_scale_y : 1.0f;
        glfwSetWindowSizeLimits(glfw,
            cfg.min_width  > 0 ? int(std::lround(cfg.min_width  * invx)) : GLFW_DONT_CARE,
            cfg.min_height > 0 ? int(std::lround(cfg.min_height * invy)) : GLFW_DONT_CARE,
            GLFW_DONT_CARE, GLFW_DONT_CARE);
    }
    win.impl_->fbo_w = 0;
    win.impl_->fbo_h = 0;
    win.resizeFbo();
#if !defined(FLUFFY_EMSCRIPTEN) && !defined(__EMSCRIPTEN__)
    if (!win.impl_->fbo) {
        return Result<Window>::err("FBO not complete");
    }
#endif
#ifdef FLUFFY_EMSCRIPTEN
    win.impl_->vg = nvgCreateGLES2(NVG_ANTIALIAS | NVG_STENCIL_STROKES);
    if (!win.impl_->vg) {
        return Result<Window>::err("nvgCreateGLES2 failed");
    }
#else
    win.impl_->vg = nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES);
    if (!win.impl_->vg) {
        return Result<Window>::err("nvgCreateGL3 failed");
    }
#endif
    // ––––––––[ default fonts ] system backend fonts — no bundled Inter.
    bool font_loaded = false;
    bool bold_loaded = false;
#if defined(FLUFFY_EMSCRIPTEN) || defined(__EMSCRIPTEN__)
    static constexpr const char* font_paths[] = {
        "/assets/Roboto-Regular.ttf",
        "/assets/Roboto-Regular.ttf",
        "/Roboto-Regular.ttf",
    };
    static constexpr const char* bold_paths_e[] = {
        "/assets/Roboto-Bold.ttf",
        "/Roboto-Bold.ttf",
    };
    // Try Emscripten preloaded assets first
    for (const char* path : font_paths) {
        if (nvgCreateFont(win.impl_->vg, "default", path) != -1) {
            font_loaded = true;
            win.impl_->loaded_faces_.insert("default");
            break;
        }
    }
    if (!font_loaded) win.impl_->loaded_faces_.insert("default");
    for (const char* path : bold_paths_e) {
        if (nvgCreateFont(win.impl_->vg, "default-bold", path) != -1) {
            bold_loaded = true;
            win.impl_->loaded_faces_.insert("default-bold");
            break;
        }
    }
    if (!bold_loaded && font_loaded) win.impl_->loaded_faces_.insert("default-bold");
    // Skip system paths on Emscripten — already handled
    static constexpr const char* font_paths_dummy[] = {nullptr};
    static constexpr const char* bold_paths_dummy[] = {nullptr};
    (void)font_paths_dummy; (void)bold_paths_dummy;
#else
    static constexpr const char* font_paths[] = {
#if defined(__APPLE__)
        "/System/Library/Fonts/SFNSText.ttf",
        "/System/Library/Fonts/SFNS.ttf",
        "/Library/Fonts/Arial.ttf",
        "/System/Library/Fonts/Helvetica.ttc",
        "/System/Library/Fonts/Supplemental/Arial.ttf",
#elif defined(_WIN32)
        "C:\\Windows\\Fonts\\segoeui.ttf",
        "C:\\Windows\\Fonts\\arial.ttf",
#else
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/TTF/DejaVuSans.ttf",
        "/usr/share/fonts/dejavu/DejaVuSans.ttf",
#endif
    };
#endif
    if (!font_loaded) {
        for (const char* path : font_paths) {
            if (nvgCreateFont(win.impl_->vg, "default", path) != -1) {
                font_loaded = true;
                win.impl_->loaded_faces_.insert("default");
                break;
            }
        }
    }
    if (!font_loaded) {
        win.impl_->loaded_faces_.insert("default");
    }
    if (!bold_loaded) {
        // Try to synthesize bold from system bold files when embedded bold missing.
        static constexpr const char* bold_paths[] = {
#if defined(__APPLE__)
            "/System/Library/Fonts/Supplemental/Arial Bold.ttf",
            "/System/Library/Fonts/HelveticaNeue.ttc",
#elif defined(_WIN32)
            "C:\\Windows\\Fonts\\segoeuib.ttf",
            "C:\\Windows\\Fonts\\arialbd.ttf",
#else
            "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
            "/usr/share/fonts/TTF/DejaVuSans-Bold.ttf",
#endif
        };
        for (const char* path : bold_paths) {
            const bool is_ttc = std::string_view(path).ends_with(".ttc");
            const int rc = is_ttc
                ? nvgCreateFontAtIndex(win.impl_->vg, "default-bold", path, 1)
                : nvgCreateFont(win.impl_->vg, "default-bold", path);
            if (rc != -1) {
                bold_loaded = true;
                win.impl_->loaded_faces_.insert("default-bold");
                break;
            }
        }
        // Final fallback: alias bold to regular so nvgFontFace never fails.
        if (!bold_loaded && font_loaded) {
            win.impl_->loaded_faces_.insert("default-bold");
        }
    }
#if defined(__APPLE__)
    id nsApp = ((id (*)(id, SEL))objc_msgSend)(
        (id)objc_getClass("NSApplication"),
        sel_registerName("sharedApplication"));
    ((void (*)(id, SEL, BOOL))objc_msgSend)(nsApp,
        sel_registerName("activateIgnoringOtherApps:"), YES);
#endif
    win.applyWindowAppearance();
    return Result<Window>(std::move(win));
}

void Window::requestClose() noexcept {
    should_close_ = true;
    if (impl_ && impl_->glfw_window) glfwSetWindowShouldClose(impl_->glfw_window, GLFW_TRUE);
}

// ––––––––[ open ] simple creation without Result, check `if (!win)`
Window Window::open(std::string title, int width, int height) {
    Config cfg;
    cfg.title = std::move(title);
    cfg.width = width;
    cfg.height = height;
    if (auto r = create(cfg)) {
        return std::move(r).value();
    }
    return Window();
}

// ––––––––[ showDialog ] preset buttons, returns the chosen index
Window::DialogChoice Window::showDialog(const std::string& title,
                                        const std::string& message,
                                        DialogButtons buttons) {
    std::vector<Inline> labels;
    switch (buttons) {
    case DialogButtons::Ok:
        labels.emplace_back("OK");
        break;
    case DialogButtons::OkCancel:
        labels.emplace_back("OK");
        labels.emplace_back("Cancel");
        break;
    case DialogButtons::YesNo:
        labels.emplace_back("Yes");
        labels.emplace_back("No");
        break;
    case DialogButtons::YesNoCancel:
        labels.emplace_back("Yes");
        labels.emplace_back("No");
        labels.emplace_back("Cancel");
        break;
    }
    return showDialog(title, Inline(message), std::move(labels));
}

// ––––––––[ showDialog ] custom plain labels, returns the chosen index
Window::DialogChoice
Window::showDialog(const std::string& title,
                   const std::string& message,
                   const std::vector<std::string>& buttonLabels) {
    std::vector<Inline> labels;
    labels.reserve(buttonLabels.size());
    for (const auto& label : buttonLabels) labels.emplace_back(label);
    return showDialog(title, Inline(message), std::move(labels));
}

// ––––––––[ showDialog ] inline labels, returns the chosen index
Window::DialogChoice Window::showDialog(const std::string& title,
                                        const std::string& message,
                                        const std::vector<Inline>& buttonContent) {
    return showDialog(title, Inline(message), buttonContent);
}

// ––––––––[ showDialog ] inline message and labels, returns the chosen index
Window::DialogChoice Window::showDialog(const std::string& title,
                                        const Inline& message,
                                        const std::vector<Inline>& buttonContent) {
    std::vector<DialogSpec> specs;
    specs.reserve(buttonContent.size());
    for (const auto& inl : buttonContent) {
        std::string label;
        for (const auto& r : inl.runs())
            if (r.type == Inline::Run::Type::Text) label += r.utf8;
        specs.push_back({std::move(label), inl});
    }
    const std::string picked = showDialogImpl(title, message, specs);
    if (picked.empty()) return DialogChoice::Dismissed;
    for (std::size_t i = 0; i < specs.size(); ++i)
        if (specs[i].label == picked)
            return static_cast<DialogChoice>(i);
    return DialogChoice::First;
}

// ––––––––[ onKey ] simple key callback without raw event plumbing
void Window::onKey(std::function<bool(int key, int mods)> h) {
    if (!h) {
        clearEventHandler();
        return;
    }
    setEventHandler([h = std::move(h)](const Event& e, Window&) -> EventResult {
        if (const auto* ke = std::get_if<KeyDownEvent>(&e)) {
            if (h(ke->key, ke->mods)) {
                return EventResult::Consumed;
            }
        }
        return EventResult::Ignored;
    });
}

// ––––––––[ onClose ] simple close interceptor, true keeps the window open
void Window::onClose(std::function<bool()> h) {
    if (!h) {
        clearEventHandler();
        return;
    }
    setEventHandler([h = std::move(h)](const Event& e, Window&) -> EventResult {
        if (std::holds_alternative<CloseEvent>(e)) {
            return h() ? EventResult::Consumed : EventResult::Ignored;
        }
        return EventResult::Ignored;
    });
}

Container& Window::verticalLayout() {
    if (!root_container_) {
        auto d = designSize();
        root_container_ = &add<Container>();
        root_container_->fillToWindow(float(d.first), float(d.second));
        root_container_->setLayout(std::make_unique<VerticalLayout>());
    }
    return *root_container_;
}

Container& Window::horizontalLayout() {
    if (!root_container_) {
        auto d = designSize();
        root_container_ = &add<Container>();
        root_container_->fillToWindow(float(d.first), float(d.second));
        root_container_->setLayout(std::make_unique<HorizontalLayout>());
    }
    return *root_container_;
}

// ––––––––[ gridLayout ] root container with grid layout of given columns
Container& Window::gridLayout(int cols) {
    if (!root_container_) {
        auto d = designSize();
        root_container_ = &add<Container>();
        root_container_->fillToWindow(float(d.first), float(d.second));
        root_container_->setLayout(std::make_unique<GridLayout>(cols));
    }
    return *root_container_;
}

std::pair<int,int> Window::designSize() const noexcept {
    if (!impl_) return {0, 0};
    return { impl_->design_w, impl_->design_h };
}

const char* Window::getClipboardString() const {
    if (!impl_ || !impl_->glfw_window) return nullptr;
    return glfwGetClipboardString(impl_->glfw_window);
}

void Window::setClipboardString(const char* str) {
    if (!impl_ || !impl_->glfw_window || !str) return;
    glfwSetClipboardString(impl_->glfw_window, str);
}

// ––––––––[ requestFocus ] blurs old focus, sets new focus.
void Window::requestFocus(Widget* w) noexcept {
    if (!w) return;
    if (w == focusWidget_) return;
    if (focusWidget_) focusWidget_->setFocused(false);
    focusWidget_ = w;
    focusWidget_->setFocused(true);
}

// ––––––––[ clearFocus ] blurs and releases the currently focused widget.
void Window::clearFocus() noexcept {
    if (focusWidget_) {
        focusWidget_->setFocused(false);
        focusWidget_ = nullptr;
    }
}

// ––––––––[ focusStep ] walks the focus chain; dir +1 forward, -1 backward.
void Window::focusStep(int dir) {
    std::vector<Widget*> chain;
    if (popupPtr_) {
        collectFocusable(popupPtr_, chain);
    } else {
        for (auto& wgt : widgets_) collectFocusable(wgt.get(), chain);
    }
    if (chain.empty()) return;
    auto it = std::find(chain.begin(), chain.end(), focusWidget_);
    std::size_t idx = 0;
    if (it == chain.end()) {
        idx = (dir > 0) ? 0 : chain.size() - 1;
    } else {
        const std::size_t cur = static_cast<std::size_t>(it - chain.begin());
        const std::size_t n = chain.size();
        idx = (cur + n + (dir > 0 ? std::size_t{1} : n - 1)) % n;
    }
    requestFocus(chain[idx]);
}

// ––––––––[ focusNext ] moves keyboard focus to the next focusable widget.
void Window::focusNext() { focusStep(+1); }

// ––––––––[ focusPrevious ] moves keyboard focus to the previous focusable widget.
void Window::focusPrevious() { focusStep(-1); }

void Window::setCursorShape(CursorShape shape) {
    if (!impl_ || !impl_->glfw_window) return;
    if (shape == impl_->currentCursorShape) return;
    impl_->currentCursorShape = shape;
    const int slot = static_cast<int>(shape);
    if (!impl_->cursors[slot])
        impl_->cursors[slot] = glfwCreateStandardCursor(glfwCursorFor(shape));
    if (impl_->cursors[slot])
        glfwSetCursor(impl_->glfw_window, impl_->cursors[slot]);
}

void Window::setTheme(const Theme& t) noexcept {
    theme_ = t;
    applyWindowAppearance();
    // Force re-layout of all root widgets so they pick up new theme metrics
    // (button_padding_x/y, text_size, toggle_size, etc.).
    if (impl_) {
        ResizeEvent re{ impl_->design_w, impl_->design_h };
        for (auto& wgt : widgets_) wgt->onResize(re);
    }
}

void Window::setEnabledButtons(WindowButtons flags) noexcept {
    enabledButtons_ = flags;
    applyWindowButtons();
}

void Window::applyWindowButtons() {
    if (!impl_ || !impl_->glfw_window) return;
#if defined(__APPLE__) || defined(_WIN32)
    const bool can_close =
        (enabledButtons_ & WindowButtons::Close) != WindowButtons::None;
    const bool can_minimize =
        (enabledButtons_ & WindowButtons::Minimize) != WindowButtons::None;
    const bool can_maximize =
        (enabledButtons_ & WindowButtons::Maximize) != WindowButtons::None;
#endif
#if defined(__APPLE__)
    id ns_window = (id)glfwGetCocoaWindow(impl_->glfw_window);
    if (!ns_window) return;
    auto set_btn = [&](long kind, bool en) {
        SEL sel = sel_registerName("standardWindowButton:");
        id btn = ((id (*)(id, SEL, long))objc_msgSend)(ns_window, sel, kind);
        if (btn)
            ((void (*)(id, SEL, BOOL))objc_msgSend)(
                btn, sel_registerName("setEnabled:"), (BOOL)(en ? YES : NO));
    };
    set_btn(0, can_close);
    set_btn(1, can_minimize);
    set_btn(2, can_maximize);
#elif defined(_WIN32)
    HWND hwnd = glfwGetWin32Window(impl_->glfw_window);
    if (!hwnd) return;
    HMENU menu = GetSystemMenu(hwnd, FALSE);
    if (menu) {
        EnableMenuItem(menu, SC_CLOSE,
                       can_close ? MF_ENABLED : (MF_GRAYED | MF_DISABLED));
        EnableMenuItem(menu, SC_MINIMIZE,
                       can_minimize ? MF_ENABLED : (MF_GRAYED | MF_DISABLED));
        EnableMenuItem(menu, SC_MAXIMIZE,
                       can_maximize ? MF_ENABLED : (MF_GRAYED | MF_DISABLED));
    }
#elif defined(__linux__)
    x11_set_decorations(impl_->glfw_window, impl_->decorated, enabledButtons_);
#endif
}

// ––––––––[ applyWindowAppearance ] sets the OS chrome to dark/light based on theme background.
void Window::applyWindowAppearance() {
    if (!impl_ || !impl_->glfw_window) return;
    const auto& bg = theme_.colors.background;
    const float lum = 0.299f * float(bg.r) + 0.587f * float(bg.g) + 0.114f * float(bg.b);
    const bool dark = lum < 128.0f;
#if defined(__APPLE__)
    id ns_window = (id)glfwGetCocoaWindow(impl_->glfw_window);
    if (!ns_window) return;
    id nsAppearance = (id)objc_getClass("NSAppearance");
    SEL appearanceNamed = sel_registerName("appearanceNamed:");
    const char* name_cstr = dark ? "NSAppearanceNameDarkAqua"
                                  : "NSAppearanceNameAqua";
    id name_str = ((id (*)(id, SEL, const char*))objc_msgSend)(
        (id)objc_getClass("NSString"),
        sel_registerName("stringWithUTF8String:"), name_cstr);
    id appearance = ((id (*)(id, SEL, id))objc_msgSend)(
        nsAppearance, appearanceNamed, name_str);
    if (appearance) {
        SEL setAppearance = sel_registerName("setAppearance:");
        ((void (*)(id, SEL, id))objc_msgSend)(ns_window, setAppearance, appearance);
    }
#elif defined(_WIN32)
    HWND hwnd = glfwGetWin32Window(impl_->glfw_window);
    if (!hwnd) return;
    HMODULE dwmapi = LoadLibraryW(L"dwmapi.dll");
    if (dwmapi) {
        using DwmSetWindowAttributeFn = HRESULT (WINAPI*)(HWND, DWORD, LPCVOID, DWORD);
        auto fn = (DwmSetWindowAttributeFn)
            GetProcAddress(dwmapi, "DwmSetWindowAttribute");
        if (fn) {
            BOOL useDark = dark ? TRUE : FALSE;
            fn(hwnd, 20, &useDark, sizeof(useDark));  // 20 = DWMWA_USE_IMMERSIVE_DARK_MODE
        }
        FreeLibrary(dwmapi);
    }
#elif defined(__linux__)
    Display* display = glfwGetX11Display();
    if (!display) return;
    ::Window x11_win = glfwGetX11Window(impl_->glfw_window);
    if (!x11_win) return;
    Atom atoms[2];
    const char* names[2] = {"_GTK_THEME_VARIANT", dark ? "dark" : "light"};
    atoms[0] = XInternAtom(display, names[0], False);
    XChangeProperty(display, x11_win, atoms[0], XA_STRING, 8,
                    PropModeReplace,
                    reinterpret_cast<const unsigned char*>(names[1]),
                    static_cast<int>(std::strlen(names[1])));
    XFlush(display);
#endif
    (void)dark;
}

Widget* Window::adopt(std::unique_ptr<Widget> w) {
    if (!w) return nullptr;
    w->setWindow(this);
    Widget* raw = w.get();
    widgets_.push_back(std::move(w));
    return raw;
}

// ––––––––[ showPopup ] adds a popup widget as the topmost overlay.
// Keeps the previous popup visible (cascading submenus).
void Window::showPopup(std::unique_ptr<Widget> popup) {
    if (popupPtr_) {
        prevPopups_.push_back(popupPtr_);
    }
    Widget* raw = popup.get();
    popup->setWindow(this);
    popupPtr_ = raw;
    widgets_.push_back(std::move(popup));
}

// ––––––––[ closePopup ] removes the current popup AND any cascaded parent popups.
void Window::closePopup() {
    if (!popupPtr_) return;
    // Collect all popup pointers to remove (current + all ancestors in cascade)
    std::vector<Widget*> to_remove;
    to_remove.push_back(popupPtr_);
    to_remove.insert(to_remove.end(), prevPopups_.begin(), prevPopups_.end());
    prevPopups_.clear();
    // If the focused widget is one of the popups being destroyed, clear it now
    // to avoid use-after-free on the next event dispatch.
    if (focusWidget_ && std::find(to_remove.begin(), to_remove.end(), focusWidget_) != to_remove.end()) {
        focusWidget_ = nullptr;
    }
    popupPtr_ = nullptr;
    auto it = std::remove_if(widgets_.begin(), widgets_.end(),
        [&](const auto& w) {
            return std::find(to_remove.begin(), to_remove.end(), w.get()) != to_remove.end();
        });
    widgets_.erase(it, widgets_.end());
}

// ––––––––[ flushDeferredPopupClose ] safely closes the popup after event dispatch.
void Window::flushDeferredPopupClose(Window* win) noexcept {
    if (win && win->popupPendingClose_) {
        win->popupPendingClose_ = false;
        win->closePopup();
    }
}

NoneResult Window::loadFromString(std::string_view yaml_content) {
    std::string buf(yaml_content);
    dyml::Directyaml dy = parseYaml(buf);
    YamlNode doc(dy);
    if (!doc.IsMap()) {
        return NoneResult::err(
            "yaml_loader: top-level YAML must be a map of widgets, "
            "e.g. `Container: ... \n Button: ...`");
    }
    for (const auto& child : doc) {
        const std::string key = child.key();
        auto wres = ::fluffy::parseWidgetEntry(key, child);
        if (wres) {
            adopt(std::move(wres).value());
        }
    }
    return NoneResult{std::monostate{}};
}

NoneResult Window::load(const std::string& filepath) {
    std::ifstream in(filepath);
    if (!in) {
        return NoneResult::err(
            std::format("yaml_loader: cannot open file '{}'", filepath));
    }
    std::stringstream buf;
    buf << in.rdbuf();
    return loadFromString(buf.str());
}

void Window::registerFontMemory(const std::string& name,
                                  const void* data,
                                  std::size_t size) {
    if (name.empty() || !impl_) return;
    auto& e = impl_->mem_fonts_[name];
    e.assign(static_cast<const unsigned char*>(data),
             static_cast<const unsigned char*>(data) + size);
    impl_->loaded_faces_.erase(name);
}

// ––––––––[ loadSystemFont ] finds and loads a system font by family name.
bool Window::loadSystemFont(const std::string& family_name) {
    if (family_name.empty() || !impl_ || !impl_->vg) return false;

    const std::string path = findSystemFont(family_name);
    if (path.empty()) return false;

    // Register under both the family name and the full path so that
    // both theme.font_family = "Helvetica" and "file:/path/Helvetica.ttc" work.
    nvgCreateFont(impl_->vg, family_name.c_str(), path.c_str());
    const int rc = nvgCreateFont(impl_->vg, path.c_str(), path.c_str());
    if (rc == -1) return false;

    impl_->loaded_faces_.insert(family_name);
    impl_->loaded_faces_.insert(path);
    return true;
}

// ––––––––[ nativeHandle ] returns NSWindow* / HWND / Window.
void* Window::nativeHandle() const noexcept {
    if (!impl_ || !impl_->glfw_window) return nullptr;
#if defined(FLUFFY_EMSCRIPTEN) || defined(__EMSCRIPTEN__)
    return nullptr;
#elif defined(__APPLE__)
    return (void*)glfwGetCocoaWindow(impl_->glfw_window);
#elif defined(_WIN32)
    return (void*)glfwGetWin32Window(impl_->glfw_window);
#else
    return (void*)(uintptr_t)glfwGetX11Window(impl_->glfw_window);
#endif
}

NVGcontext* Window::nvgContext() const noexcept {
    return impl_ ? impl_->vg : nullptr;
}

bool Window::prepareFont(NVGcontext* vg,
                          const std::string& spec,
                          std::string& out_handle) const {
    out_handle.clear();
    if (!impl_ || spec.empty()) { out_handle = "default"; return false; }
    bool is_file = spec.rfind("file:", 0) == 0;
    const bool is_mem  = spec.rfind("mem:",  0) == 0;
    // Auto-detect bare file paths by extension (.ttf, .ttc, .otf)
    if (!is_file && !is_mem) {
        const auto dot = spec.rfind('.');
        if (dot != std::string::npos) {
            const auto ext = spec.substr(dot);
            if (ext == ".ttf" || ext == ".ttc" || ext == ".otf") {
                is_file = true;
            }
        }
    }
    std::string handle;
    if (is_file) {
        handle = (spec.rfind("file:", 0) == 0) ? spec.substr(5) : spec;
    } else if (is_mem) {
        handle = spec.substr(4);
    } else {
        handle = spec;
    }
    if (impl_->loaded_faces_.count(handle)) { out_handle = handle; return true; }
    int rc = -1;
    if (is_file) {
        rc = nvgCreateFont(vg, handle.c_str(), handle.c_str());
    } else if (is_mem) {
        auto it = impl_->mem_fonts_.find(handle);
        if (it != impl_->mem_fonts_.end()) {
            rc = nvgCreateFontMem(vg, handle.c_str(), it->second.data(),
                                  static_cast<int>(it->second.size()), 0);
        } else {
            // Lazy-load: read the file from disk into mem_fonts_ on first use
            std::ifstream ifs(handle, std::ios::binary | std::ios::ate);
            if (ifs.is_open()) {
                const auto sz = ifs.tellg();
                ifs.seekg(0);
                auto& buf = impl_->mem_fonts_[handle];
                buf.resize(static_cast<size_t>(sz));
                ifs.read(reinterpret_cast<char*>(buf.data()), sz);
                rc = nvgCreateFontMem(vg, handle.c_str(), buf.data(),
                                      static_cast<int>(buf.size()), 0);
                if (rc == -1) {
                    impl_->mem_fonts_.erase(handle);
                }
            }
        }
    }
    if (rc == -1) { out_handle = "default"; return false; }
    out_handle = handle;
    impl_->loaded_faces_.insert(std::move(handle));
    return true;
}

#if defined(FLUFFY_EMSCRIPTEN) || defined(__EMSCRIPTEN__)
Window* Window::g_emscriptenWindow = nullptr;
void Window::emscriptenMainLoop() {
    if (!Window::g_emscriptenWindow || !Window::g_emscriptenWindow->impl_) return;
    auto* win = Window::g_emscriptenWindow;
    if (glfwWindowShouldClose(win->impl_->glfw_window) || win->should_close_) {
        emscripten_cancel_main_loop();
        return;
    }
    win->emscriptenFrame();
}
void Window::emscriptenFrame() {
    if (!impl_) return;
    auto& im = *impl_;
    // Update window size
    {
        int lw = 0, lh = 0, fbw = 0, fbh = 0;
        glfwGetWindowSize(im.glfw_window, &lw, &lh);
        glfwGetFramebufferSize(im.glfw_window, &fbw, &fbh);
        im.logical_w = lw; im.logical_h = lh;
        im.fb_w = fbw; im.fb_h = fbh;
        im.scale_x = lw > 0 ? float(fbw) / float(lw) : 1.0f;
        im.scale_y = lh > 0 ? float(fbh) / float(lh) : 1.0f;
        // Keep design_w/h as cfg size (920x640), not CSS size, for correct layout
        im.fbo_w = fbw;
        im.fbo_h = fbh;
    }
    // Update hover states
    {
        setCursorShape(CursorShape::Arrow);
        float mx{}, my{};
        cursorPosition(mx, my);
        for (auto& wgt : widgets_)
            if (wgt->visible() && wgt->enabled())
                wgt->updateHover(mx, my);
        {
            Widget* hovered_tip = nullptr;
            std::string tip_text;
            Inline tip_inline;
            std::function<void(Widget*)> find_tip = [&](Widget* w) {
                if (!w || !w->visible() || !w->enabled()) return;
                if (w->containsPoint(mx, my)) {
                    Inline inl = getInlineTooltip(w);
                    if (!inl.empty()) { hovered_tip = w; tip_inline = inl; return; }
                    if (!w->userdata().empty()) { hovered_tip = w; tip_text = w->userdata(); return; }
                }
                w->forEachChild([&](Widget* child){ if (!hovered_tip) find_tip(child); });
            };
            for (auto& wgt : widgets_) { if (hovered_tip) break; find_tip(wgt.get()); }
            double now = glfwGetTime();
            if (hovered_tip) {
                if (hovered_tip != tooltip_hover_widget_) { tooltip_hover_widget_ = hovered_tip; tooltip_hover_start_ = now; }
                else if (!hasPopup() && (now - tooltip_hover_start_) > 0.5) {
                    if (!tip_inline.empty()) Tooltip::showFor(hovered_tip, tip_inline, *this);
                    else Tooltip::showFor(hovered_tip, tip_text, *this);
                }
            } else { tooltip_hover_widget_ = nullptr; tooltip_hover_start_ = 0; }
        }
    }
    // Tick animations
    {
        const double now = glfwGetTime();
        float dt = static_cast<float>(now - last_time_);
        if (dt < 0.0f) dt = 0.0f; if (dt > 0.050f) dt = 0.050f;
        last_time_ = now;
        for (auto& wgt : widgets_) if (wgt->visible() && wgt->enabled()) wgt->tick(dt, theme_);
    }
    renderFrame();
}
#endif

// ––––––––[ run ] GLFW event loop — single-window compat, delegates to Application if multiple windows.
void Window::run() {
    if (!impl_) return;
    // If more than one window is registered, delegate to global Application loop.
    if (auto* app = Application::tryGet()) {
        if (app->windowCount() > 1) {
            app->run();
            return;
        }
    }
    auto& im = *impl_;
    glfwSetWindowUserPointer     (im.glfw_window, this);
    glfwSetCursorPosCallback     (im.glfw_window, &Window::s_cursor_pos_cb);
    glfwSetMouseButtonCallback   (im.glfw_window, &Window::s_mouse_btn_cb);
    glfwSetFramebufferSizeCallback(im.glfw_window, &Window::s_framebuffer_size_cb);
    glfwSetKeyCallback           (im.glfw_window, &Window::s_key_cb);
    glfwSetCharCallback          (im.glfw_window, &Window::s_char_cb);
    glfwSetScrollCallback       (im.glfw_window, &Window::s_scroll_cb);
    glfwSetWindowCloseCallback   (im.glfw_window, &Window::s_close_cb);
    // Apply layout once BEFORE the event loop so widget bounds are correct
    // before any GLFW callbacks fire on the first frame.
    {
        syncSize();
        for (auto& wgt : widgets_) {
            if (wgt->fillsParent())
                wgt->fillToWindow(float(im.design_w), float(im.design_h));
        }
        ResizeEvent init_layout{ im.design_w, im.design_h };
        for (auto& wgt : widgets_) wgt->onResize(init_layout);
    }
#if defined(FLUFFY_EMSCRIPTEN) || defined(__EMSCRIPTEN__)
    Window::g_emscriptenWindow = this;
    emscripten_set_main_loop(Window::emscriptenMainLoop, 0, true);
#else
    while (!glfwWindowShouldClose(im.glfw_window) && !should_close_) {
        glfwPollEvents();
        // Update window size
        {
            syncSize();
        }
        // Update hover states
        {
            setCursorShape(CursorShape::Arrow);
            float mx{}, my{};
            cursorPosition(mx, my);
            for (auto& wgt : widgets_)
                if (wgt->visible() && wgt->enabled())
                    wgt->updateHover(mx, my);
            // Default hover tooltip — show after 0.5s hover (recursive search)
            {
                Widget* hovered_tip = nullptr;
                std::string tip_text;
                Inline tip_inline;
                std::function<void(Widget*)> find_tip = [&](Widget* w) {
                    if (!w || !w->visible() || !w->enabled()) return;
                    if (w->containsPoint(mx, my)) {
                    Inline inl = getInlineTooltip(w);
                        if (!inl.empty()) {
                            hovered_tip = w;
                            tip_inline = inl;
                            return;
                        }
                        if (!w->userdata().empty()) {
                            hovered_tip = w;
                            tip_text = w->userdata();
                            return;
                        }
                    }
                    w->forEachChild([&](Widget* child){ if (!hovered_tip) find_tip(child); });
                };
                for (auto& wgt : widgets_) {
                    if (hovered_tip) break;
                    find_tip(wgt.get());
                }
                double now = glfwGetTime();
                if (hovered_tip) {
                    if (hovered_tip != tooltip_hover_widget_) {
                        tooltip_hover_widget_ = hovered_tip;
                        tooltip_hover_start_ = now;
                    } else if (!hasPopup() && (now - tooltip_hover_start_) > 0.5) {
                        if (!tip_inline.empty()) Tooltip::showFor(hovered_tip, tip_inline, *this);
                        else Tooltip::showFor(hovered_tip, tip_text, *this);
                    }
                } else {
                    tooltip_hover_widget_ = nullptr;
                    tooltip_hover_start_ = 0;
                }
            }
        }
        // Tick animations
        {
            const double now = glfwGetTime();
            float dt = static_cast<float>(now - last_time_);
            if (dt < 0.0f)   dt = 0.0f;
            if (dt > 0.050f) dt = 0.050f;
            last_time_ = now;
            for (auto& wgt : widgets_)
                if (wgt->visible() && wgt->enabled())
                    wgt->tick(dt, theme_);
        }
        renderFrame();
    }
#endif
}

namespace {
void set_window_movable(GLFWwindow* win, bool movable) {
    if (!win) return;
#if defined(__APPLE__)
    id ns_window = (id)glfwGetCocoaWindow(win);
    if (ns_window) {
        SEL setMovable      = sel_registerName("setMovable:");
        SEL setMovableByBg  = sel_registerName("setMovableByWindowBackground:");
        auto send = (void (*)(id, SEL, BOOL))objc_msgSend;
        send(ns_window, setMovable,     (BOOL)(movable ? YES : NO));
        send(ns_window, setMovableByBg, (BOOL)(movable ? YES : NO));
    }
#elif defined(__linux__)
    Display* display = glfwGetX11Display();
    if (!display) return;
    ::Window x11_win = glfwGetX11Window(win);
    if (!x11_win) return;
    Atom wm_type = XInternAtom(display, "_NET_WM_WINDOW_TYPE", False);
    Atom target  = XInternAtom(display,
        movable ? "_NET_WM_WINDOW_TYPE_NORMAL"
                : "_NET_WM_WINDOW_TYPE_DIALOG",
        False);
    XChangeProperty(display, x11_win, wm_type, XA_ATOM, 32,
                    PropModeReplace, (unsigned char*)&target, 1);
    XFlush(display);
#endif
    (void)win; (void)movable;
}

#if defined(__APPLE__)
static BOOL nonKey_canBecomeKeyWindow(id self, SEL _cmd) {
    (void)self; (void)_cmd; return NO;
}
static BOOL nonKey_canBecomeMainWindow(id self, SEL _cmd) {
    (void)self; (void)_cmd; return NO;
}
#endif

void set_window_enabled(GLFWwindow* win, bool enabled) {
    if (!win) return;
#if defined(__APPLE__)
    id ns_window = (id)glfwGetCocoaWindow(win);
    if (!ns_window) return;
    static Class nonFocusableClass = []{
        Class base = (Class)objc_getClass("NSWindow");
        if (!base) return (Class)nullptr;
        Class subclass = objc_allocateClassPair(base, "Fluffy_NonFocusableWindow", 0);
        if (!subclass) {
            return objc_getClass("Fluffy_NonFocusableWindow");
        }
        class_addMethod(subclass, sel_registerName("canBecomeKeyWindow"),
                        (IMP)nonKey_canBecomeKeyWindow, "B@:");
        class_addMethod(subclass, sel_registerName("canBecomeMainWindow"),
                        (IMP)nonKey_canBecomeMainWindow, "B@:");
        objc_registerClassPair(subclass);
        return subclass;
    }();
    if (!nonFocusableClass) return;
    struct Saved { GLFWwindow* key; Class val; };
    static Saved saved = {nullptr, nullptr};
    if (!enabled) {
        saved.key = win;
        saved.val = object_getClass(ns_window);
        object_setClass(ns_window, nonFocusableClass);
    } else if (saved.key == win && saved.val) {
        object_setClass(ns_window, saved.val);
        saved = {nullptr, nullptr};
    }
#elif defined(_WIN32)
    HWND hwnd = glfwGetWin32Window(win);
    if (hwnd) EnableWindow(hwnd, enabled ? TRUE : FALSE);
#endif
    (void)win; (void)enabled;
}
}  

// ––––––––[ dialogFrame ] event-processing frame for popup (uses queue internally).
void Window::dialogFrame(Window& parent, Window& popup) {
    auto* pg = parent.impl_->glfw_window;
    auto* pp = popup.impl_->glfw_window;
    glfwPollEvents();
    parent.syncSize();
    popup.syncSize();
    {
        popup.setCursorShape(CursorShape::Arrow);
        float mx, my;
        popup.cursorPosition(mx, my);
        for (auto& wgt : popup.widgets_)
            if (wgt->visible() && wgt->enabled()) wgt->updateHover(mx, my);
    }
    {
        auto& evts = popup.pending_events_;
        for (auto& e : evts) {
            bool consumed = false;
            for (auto it = popup.widgets_.rbegin(); it != popup.widgets_.rend(); ++it)
                if ((*it)->visible() && (*it)->enabled()
                    && (*it)->handleEvent(e, popup) == EventResult::Consumed)
                { consumed = true; break; }
            if (!consumed)
                if (auto* re = std::get_if<ResizeEvent>(&e); re && popup.onResize_)
                    popup.onResize_(re->width, re->height);
        }
        evts.clear();
    }
    {
        const double now = glfwGetTime();
        float dt = static_cast<float>(now - popup.last_time_);
        if (dt < 0.0f) dt = 0.0f;
        if (dt > 0.050f) dt = 0.050f;
        popup.last_time_ = now;
        for (auto& wgt : popup.widgets_)
            if (wgt->visible() && wgt->enabled()) wgt->tick(dt, popup.theme_);
    }
    glfwMakeContextCurrent(pg);
    parent.renderFrame();
    glfwMakeContextCurrent(pp);
    popup.renderFrame();
}

std::string
Window::showDialogImpl(const std::string& title,
                        const Inline& message,
                        const std::vector<DialogSpec>& specs) {
#if defined(FLUFFY_EMSCRIPTEN) || defined(__EMSCRIPTEN__)
    // Browser fallback — no second GLFW window
    std::string msgStr;
    for (auto& r : message.runs()) if (r.type == Inline::Run::Type::Text) msgStr += r.utf8;
    std::string combined = title + "\n" + msgStr;
    if (specs.size() == 1) {
        EM_ASM({ alert(UTF8ToString($0)); }, combined.c_str());
        return specs[0].label;
    } else if (specs.size() == 2) {
        int res = EM_ASM_INT({ return confirm(UTF8ToString($0)) ? 1 : 0; }, combined.c_str());
        return res ? specs[0].label : specs[1].label;
    } else if (specs.size() >= 3) {
        // For 3 buttons, show alert and return first
        EM_ASM({ alert(UTF8ToString($0)); }, combined.c_str());
        return specs[0].label;
    } else {
        return {};
    }
#endif
    if (!impl_) { return {}; }
    const int popup_w = 380;
    const int popup_h = 180;
    Window::Config cfg;
    cfg.title     = title;
    cfg.width     = popup_w;
    cfg.height    = popup_h;
    cfg.resizable = false;
    cfg.theme     = theme_;
    auto popup_or_err = Window::create(cfg);
    if (!popup_or_err) {
        return {};
    }
    std::string result;
    {
        Window popup = std::move(popup_or_err).value();
        {
            int px = 0, py = 0, pw = 0, ph = 0, cw = 0, ch = 0;
            glfwGetWindowPos (impl_->glfw_window, &px, &py);
            glfwGetWindowSize(impl_->glfw_window, &pw, &ph);
            glfwGetWindowSize(popup.impl_->glfw_window, &cw, &ch);
            glfwSetWindowPos(popup.impl_->glfw_window,
                             px + (pw - cw) / 2,
                             py + (ph - ch) / 2);
        }
        const int saved_resizable = glfwGetWindowAttrib(impl_->glfw_window, GLFW_RESIZABLE);
        glfwSetWindowAttrib(impl_->glfw_window, GLFW_RESIZABLE, GLFW_FALSE);
        set_window_enabled(impl_->glfw_window, false);
        set_window_movable(impl_->glfw_window, false);
        glfwSetWindowAttrib(popup.impl_->glfw_window, GLFW_FLOATING, GLFW_TRUE);
        popup.setEnabledButtons(WindowButtons::Close);
        const WindowButtons saved_parent_buttons = enabledButtons_;
        setEnabledButtons(WindowButtons::None);
        for (auto& wgt : widgets_) wgt->clearFocus();
        focusWidget_ = nullptr;
        for (auto& wgt : widgets_)
            if (wgt->visible() && wgt->enabled()) wgt->updateHover(-10000.0f, -10000.0f);
        // Build dialog content
        auto& root = popup.add<Container>(
            Rect(0.0f, 0.0f, float(popup_w), float(popup_h)));
        {
            auto vl = std::make_unique<VerticalLayout>();
            vl->setPadding(16.0f); vl->setSpacing(8.0f);
            root.setLayout(std::move(vl));
        }
        {
            auto lbl = std::make_unique<Label>(Rect{}, title, Label::Align::Center);
            lbl->setTextSize(18.0f);
            root.addChild(std::move(lbl));
            root.addChild(std::make_unique<Label>(Rect{}, message, Label::Align::Center));
            auto spacer = std::make_unique<Container>(Rect{});
            spacer->setAutoSize(AutoSize::Disabled);
            root.addChild(std::move(spacer));
        }
        {
            auto btn_row = std::make_unique<Container>(Rect{0, 0, float(popup_w) - 32, 36});
            auto hl = std::make_unique<HorizontalLayout>();
            hl->setSpacing(8.0f).justify(Justify::Center);
            btn_row->setLayout(std::move(hl));
            for (const auto& spec : specs) {
                auto btn = std::make_unique<Button>(Rect{}, spec.label);
                if (!spec.inline_content.empty()) btn->setInline(spec.inline_content);
                btn->setOnClick([&result, &popup, lbl = spec.label] {
                    result = lbl;
                    popup.requestClose();
                });
                btn_row->addChild(std::move(btn));
            }
            root.addChild(std::move(btn_row));
        }
        glfwSetWindowUserPointer(popup.impl_->glfw_window, &popup);
        glfwSetCursorPosCallback(popup.impl_->glfw_window, &Window::s_cursor_pos_cb);
        glfwSetMouseButtonCallback(popup.impl_->glfw_window, &Window::s_mouse_btn_cb);
        glfwSetKeyCallback(popup.impl_->glfw_window, &Window::s_key_cb);
        glfwSetCharCallback(popup.impl_->glfw_window, &Window::s_char_cb);
        glfwSetWindowCloseCallback(popup.impl_->glfw_window, &Window::s_close_cb);
        while (!glfwWindowShouldClose(popup.impl_->glfw_window)
               && !popup.should_close_ && !should_close_)
            Window::dialogFrame(*this, popup);
        set_window_enabled(impl_->glfw_window, true);
        glfwSetWindowAttrib(impl_->glfw_window, GLFW_RESIZABLE, saved_resizable);
        set_window_movable(impl_->glfw_window, true);
        setEnabledButtons(saved_parent_buttons);
        glfwMakeContextCurrent(popup.impl_->glfw_window);
    }
    glfwMakeContextCurrent(impl_->glfw_window);
    glfwFocusWindow(impl_->glfw_window);
    return result;
}

// ––––––––[ resizeFbo ] recreate the FBO at the current design_w/design_h × design_px_ratio.
void Window::resizeFbo() {
    auto& im = *impl_;
#if defined(FLUFFY_EMSCRIPTEN) || defined(__EMSCRIPTEN__)
    // Emscripten/WebGL: render directly to default framebuffer, no FBO
    im.fbo = 0; im.fbo_color = 0; im.fbo_stencil = 0;
    im.fbo_w = im.fb_w; im.fbo_h = im.fb_h;
    return;
#else
    if (im.fbo)         { glDeleteFramebuffers  (1, &im.fbo);         im.fbo         = 0; }
    if (im.fbo_color)   { glDeleteTextures      (1, &im.fbo_color);   im.fbo_color   = 0; }
    if (im.fbo_stencil) { glDeleteRenderbuffers (1, &im.fbo_stencil); im.fbo_stencil = 0; }
    im.fbo_w = int(float(im.design_w) * im.design_px_ratio);
    im.fbo_h = int(float(im.design_h) * im.design_px_ratio);
    if (im.fbo_w <= 0 || im.fbo_h <= 0) return;
    glGenFramebuffers(1, &im.fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, im.fbo);
    glGenTextures(1, &im.fbo_color);
    glBindTexture(GL_TEXTURE_2D, im.fbo_color);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
                 im.fbo_w, im.fbo_h,
                 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, im.fbo_color, 0);
    glGenRenderbuffers(1, &im.fbo_stencil);
    glBindRenderbuffer(GL_RENDERBUFFER, im.fbo_stencil);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_STENCIL_INDEX8,
                          im.fbo_w, im.fbo_h);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT,
                              GL_RENDERBUFFER, im.fbo_stencil);
    const GLenum fb_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    if (fb_status != GL_FRAMEBUFFER_COMPLETE) {
        glDeleteFramebuffers(1, &im.fbo);         im.fbo         = 0;
        glDeleteTextures(1, &im.fbo_color);       im.fbo_color   = 0;
        glDeleteRenderbuffers(1, &im.fbo_stencil); im.fbo_stencil = 0;
    }
#endif
}

// ––––––––[ saveScreenshot ] reads the FBO and writes a PNG file.
bool Window::saveScreenshot(const std::string& path) const {
#if defined(FLUFFY_EMSCRIPTEN) || defined(__EMSCRIPTEN__)
    (void)path;
    return false;
#else
    if (!impl_ || !impl_->fbo) return false;
    auto& im = *impl_;
    if (im.fbo_w <= 0 || im.fbo_h <= 0) return false;
    glBindFramebuffer(GL_FRAMEBUFFER, im.fbo);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    std::vector<unsigned char> pixels(static_cast<std::size_t>(im.fbo_w) * im.fbo_h * 4);
    glReadPixels(0, 0, im.fbo_w, im.fbo_h, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    if (pixels.empty()) return false;
    stbi_flip_vertically_on_write(1);
    const int ok = stbi_write_png(path.c_str(), im.fbo_w, im.fbo_h, 4, pixels.data(), im.fbo_w * 4);
    return ok != 0;
#endif
}

// ––––––––[ glfwWindow ] returns GLFWwindow* for Application
GLFWwindow* Window::glfwWindow() const noexcept {
    return impl_ ? impl_->glfw_window : nullptr;
}

// ––––––––[ isAlive ] true if window not closed
bool Window::isAlive() const noexcept {
    if (!impl_ || !impl_->glfw_window) return false;
    return !glfwWindowShouldClose(impl_->glfw_window) && !should_close_;
}

// ––––––––[ syncSize ] updates logical/fb size from GLFW
void Window::syncSize() {
    if (!impl_ || !impl_->glfw_window) return;
    auto& im = *impl_;
    int lw = 0, lh = 0, fbw = 0, fbh = 0;
    glfwGetWindowSize(im.glfw_window, &lw, &lh);
    glfwGetFramebufferSize(im.glfw_window, &fbw, &fbh);
    float csx = 1.0f, csy = 1.0f;
#if !defined(FLUFFY_EMSCRIPTEN) && !defined(__EMSCRIPTEN__)
    glfwGetWindowContentScale(im.glfw_window, &csx, &csy);
#endif
    if (!(csx > 0.0f)) csx = 1.0f;
    if (!(csy > 0.0f)) csy = 1.0f;
#if defined(__linux__)
    if (im.forced_scale_x > 0.0f) csx = im.forced_scale_x;
    if (im.forced_scale_y > 0.0f) csy = im.forced_scale_y;
#endif
    im.logical_w = lw;
    im.logical_h = lh;
    im.fb_w = fbw;
    im.fb_h = fbh;
    im.scale_x = csx;
    im.scale_y = csy;
    int dw = static_cast<int>(std::lround(static_cast<double>(fbw) / static_cast<double>(csx)));
    int dh = static_cast<int>(std::lround(static_cast<double>(fbh) / static_cast<double>(csy)));
    if (dw <= 0) dw = fbw > 0 ? fbw : 1;
    if (dh <= 0) dh = fbh > 0 ? fbh : 1;
    im.design_w = dw;
    im.design_h = dh;
    im.design_px_ratio = static_cast<float>(fbw) / static_cast<float>(dw);
    im.coord_scale_x = lw > 0 ? static_cast<float>(dw) / static_cast<float>(lw) : 1.0f;
    im.coord_scale_y = lh > 0 ? static_cast<float>(dh) / static_cast<float>(lh) : 1.0f;
}

// ––––––––[ doHoverTick ] updates hover, tooltip and tick
void Window::doHoverTick(float dt) {
    if (!impl_) return;
    setCursorShape(CursorShape::Arrow);
    float mx{}, my{};
    cursorPosition(mx, my);
    for (auto& wgt : widgets_) {
        if (wgt->visible() && wgt->enabled()) wgt->updateHover(mx, my);
    }
    // Tooltip 0.5s hover search
    {
        Widget* hovered_tip = nullptr;
        std::string tip_text;
        Inline tip_inline;
        std::function<void(Widget*)> find_tip = [&](Widget* w) {
            if (!w || !w->visible() || !w->enabled()) return;
            if (w->containsPoint(mx, my)) {
                Inline inl = getInlineTooltip(w);
                if (!inl.empty()) {
                    hovered_tip = w;
                    tip_inline = inl;
                    return;
                }
                if (!w->userdata().empty()) {
                    hovered_tip = w;
                    tip_text = w->userdata();
                    return;
                }
            }
            w->forEachChild([&](Widget* child) {
                if (!hovered_tip) find_tip(child);
            });
        };
        for (auto& wgt : widgets_) {
            if (hovered_tip) break;
            find_tip(wgt.get());
        }
        double now = glfwGetTime();
        if (hovered_tip) {
            if (hovered_tip != tooltip_hover_widget_) {
                tooltip_hover_widget_ = hovered_tip;
                tooltip_hover_start_ = now;
            } else if (!hasPopup() && (now - tooltip_hover_start_) > 0.5) {
                if (!tip_inline.empty()) Tooltip::showFor(hovered_tip, tip_inline, *this);
                else Tooltip::showFor(hovered_tip, tip_text, *this);
            }
        } else {
            tooltip_hover_widget_ = nullptr;
            tooltip_hover_start_ = 0;
        }
    }
    if (dt < 0.0f) dt = 0.0f;
    if (dt > 0.050f) dt = 0.050f;
    for (auto& wgt : widgets_) {
        if (wgt->visible() && wgt->enabled()) wgt->tick(dt, theme_);
    }
}

// ––––––––[ renderFrame ] blit FBO → screen buffer, swap GLFW buffers.
void Window::renderFrame() {
    if (!impl_) return;
    auto& im = *impl_;
    if (im.fb_w <= 0 || im.fb_h <= 0) return;
#if defined(FLUFFY_EMSCRIPTEN) || defined(__EMSCRIPTEN__)
    // WebGL: render directly to default FBO (no blit)
    if (im.fbo_w <= 0 || im.fbo_h <= 0) { im.fbo_w = im.fb_w; im.fbo_h = im.fb_h; }
    const auto bg_r = float(theme_.colors.background.r) / 255.0f;
    const auto bg_g = float(theme_.colors.background.g) / 255.0f;
    const auto bg_b = float(theme_.colors.background.b) / 255.0f;
    const auto bg_a = float(theme_.colors.background.a) / 255.0f;
    glViewport(0, 0, im.fb_w, im.fb_h);
    glClearColor(bg_r, bg_g, bg_b, bg_a);
    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    nvgBeginFrame(im.vg, float(im.design_w), float(im.design_h), im.design_px_ratio);
    for (auto& wgt : widgets_) if (wgt->visible()) wgt->draw(im.vg, theme_, *this);
    nvgEndFrame(im.vg);
    glfwSwapBuffers(im.glfw_window);
    return;
#else
    if (im.fbo_w <= 0 || im.fbo_h <= 0) return;
    const auto bg_r = float(theme_.colors.background.r) / 255.0f;
    const auto bg_g = float(theme_.colors.background.g) / 255.0f;
    const auto bg_b = float(theme_.colors.background.b) / 255.0f;
    const auto bg_a = float(theme_.colors.background.a) / 255.0f;
    glBindFramebuffer(GL_FRAMEBUFFER, im.fbo);
    glViewport(0, 0, im.fbo_w, im.fbo_h);
    glClearColor(bg_r, bg_g, bg_b, bg_a);
    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    nvgBeginFrame(im.vg, float(im.design_w), float(im.design_h), im.design_px_ratio);
    for (auto& wgt : widgets_) {
        if (wgt->visible()) wgt->draw(im.vg, theme_, *this);
    }
    nvgEndFrame(im.vg);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, im.fb_w, im.fb_h);
    glClearColor(bg_r, bg_g, bg_b, bg_a);
    glClear(GL_COLOR_BUFFER_BIT);
    const int blit_w = (im.fb_w < im.fbo_w) ? im.fb_w : im.fbo_w;
    const int blit_h = (im.fb_h < im.fbo_h) ? im.fb_h : im.fbo_h;
    const int src_y0 = im.fbo_h - blit_h;
    const int src_y1 = im.fbo_h;
    const int dst_y0 = im.fb_h  - blit_h;
    const int dst_y1 = im.fb_h;
    glBindFramebuffer(GL_READ_FRAMEBUFFER, im.fbo);
    glBlitFramebuffer(
        0, src_y0, blit_w, src_y1,
        0, dst_y0, blit_w, dst_y1,
        GL_COLOR_BUFFER_BIT,
        GL_NEAREST);
    glfwSwapBuffers(im.glfw_window);
#endif
}
}  
