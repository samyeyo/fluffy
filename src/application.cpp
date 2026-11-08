// src/application.cpp
// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
// Application — static lazy singleton: GLFW lifetime, window registry, global
// event loop and manual update pumping.
//
// MIT License - Copyright (c) 2026 - Samir Tine
// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

#include <fluffy/application.hpp>

#include <fluffy/tooltip.hpp>
#include <fluffy/window.hpp>

#if defined(__EMSCRIPTEN__) || defined(FLUFFY_EMSCRIPTEN)
#include <emscripten/emscripten.h>
#endif
#include <GLFW/glfw3.h>

#include <algorithm>
#include <atomic>
#include <functional>
#include <string>
#include <vector>

#if defined(__linux__)
#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#endif

namespace fluffy {

#if defined(__linux__)
namespace {
// ––––––––[ libdecor_available ] can GLFW draw client-side Wayland chrome?
bool libdecor_available() {
    void* handle = dlopen("libdecor-0.so.0", RTLD_LAZY | RTLD_LOCAL);
    if (!handle) return false;
    dlclose(handle);
    return true;
}

// ––––––––[ configure_glfw_platform ] picks a backend that can show a title bar.
void configure_glfw_platform() {
    if (const char* requested = std::getenv("FLUFFY_PLATFORM")) {
        if (std::strcmp(requested, "x11") == 0)
            glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
        else if (std::strcmp(requested, "wayland") == 0)
            glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_WAYLAND);
        return;
    }

    const char* session = std::getenv("XDG_SESSION_TYPE");
    const bool wayland_session =
        std::getenv("WAYLAND_DISPLAY") != nullptr ||
        (session && std::strcmp(session, "wayland") == 0);
    // Without libdecor a Wayland toplevel has no title bar or buttons. Fall
    // back to XWayland, where the window manager provides native chrome.
    if (!wayland_session || libdecor_available()) return;
    if (!std::getenv("DISPLAY")) return;
    if (glfwPlatformSupported(GLFW_PLATFORM_X11))
        glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
}
}  // namespace
#endif

// ––––––––[ Application::Impl ] window registry + quit flag + GLFW ref
struct Application::Impl {
    std::vector<Window*> windows_;
    bool shouldQuit_ = false;
    double lastTime_ = 0.0;
    std::atomic<int> glfwRefCount_{0};
    bool glfwInited_ = false;
};

static Application* s_instance = nullptr;

#if defined(FLUFFY_EMSCRIPTEN) || defined(__EMSCRIPTEN__)
static Application* g_emscriptenApp = nullptr;
#endif

// ––––––––[ Application ] private ctor — glfwInit once
Application::Application()
    : impl_(new Impl()) {
    s_instance = this;
    if (impl_->glfwRefCount_.fetch_add(1) == 0) {
#if defined(__linux__)
        configure_glfw_platform();
#endif
        if (glfwInit() == GLFW_TRUE) {
            impl_->glfwInited_ = true;
        }
    }
#if defined(FLUFFY_EMSCRIPTEN) || defined(__EMSCRIPTEN__)
    g_emscriptenApp = this;
#endif
}

// ––––––––[ ~Application ] glfwTerminate when last instance
Application::~Application() {
    if (impl_) {
        if (impl_->glfwRefCount_.fetch_sub(1) == 1) {
            if (impl_->glfwInited_) {
                glfwTerminate();
            }
        }
        delete impl_;
        impl_ = nullptr;
    }
    if (s_instance == this) {
        s_instance = nullptr;
    }
#if defined(FLUFFY_EMSCRIPTEN) || defined(__EMSCRIPTEN__)
    if (g_emscriptenApp == this) {
        g_emscriptenApp = nullptr;
    }
#endif
}

// ––––––––[ get ] lazy Meyer's singleton
Application& Application::get() {
    static Application inst;
    return inst;
}

// ––––––––[ tryGet ] non-creating accessor
Application* Application::tryGet() noexcept {
    return s_instance;
}

// ––––––––[ windowCount ] number of live registered windows
std::size_t Application::windowCount() const noexcept {
    if (!impl_) return 0;
    return impl_->windows_.size();
}

// ––––––––[ window ] nth window or nullptr
Window* Application::window(std::size_t i) const noexcept {
    if (!impl_ || i >= impl_->windows_.size()) return nullptr;
    return impl_->windows_[i];
}

// ––––––––[ registerWindow ] called by Window after successful create
void Application::registerWindow(Window* w) {
    if (!w || !impl_) return;
    auto& v = impl_->windows_;
    if (std::find(v.begin(), v.end(), w) != v.end()) return;
    v.push_back(w);
}

// ––––––––[ unregisterWindow ] called by Window dtor / move
void Application::unregisterWindow(Window* w) noexcept {
    if (!w || !impl_) return;
    auto& v = impl_->windows_;
    auto it = std::find(v.begin(), v.end(), w);
    if (it != v.end()) v.erase(it);
}

// ––––––––[ shouldQuit ] true after quit()
bool Application::shouldQuit() const noexcept {
    return impl_ ? impl_->shouldQuit_ : false;
}

// ––––––––[ quit ] requests all windows to close and exits run()
void Application::quit() noexcept {
    if (!impl_) return;
    impl_->shouldQuit_ = true;
    for (Window* w : impl_->windows_) {
        if (w) w->requestClose();
    }
}

// ––––––––[ update ] pump one frame for all windows with explicit delta
bool Application::update(float delaySeconds) {
    if (!impl_ || impl_->shouldQuit_) return false;
    if (impl_->windows_.empty()) return false;

    bool anyOpen = false;
    for (Window* w : impl_->windows_) {
        if (w && w->isAlive()) { anyOpen = true; break; }
    }
    if (!anyOpen) return false;

    glfwPollEvents();

    // Hide OS windows that were just closed — prevents frozen visible window
    for (Window* w : impl_->windows_) {
        if (!w) continue;
        if (!w->isAlive()) {
            if (GLFWwindow* gw = w->glfwWindow()) {
                if (glfwGetWindowAttrib(gw, GLFW_VISIBLE)) glfwHideWindow(gw);
            }
        }
    }

    float dt = delaySeconds;
    if (dt < 0.0f) dt = 0.0f;
    if (dt > 0.050f) dt = 0.050f;

    for (Window* w : impl_->windows_) {
        if (!w || !w->isAlive()) continue;
        w->syncSize();
        w->doHoverTick(dt);
    }

    for (Window* w : impl_->windows_) {
        if (!w || !w->isAlive()) continue;
        glfwMakeContextCurrent(w->glfwWindow());
        w->renderFrame();
    }

    if (!impl_->windows_.empty() && impl_->windows_[0] && impl_->windows_[0]->glfwWindow()) {
        glfwMakeContextCurrent(impl_->windows_[0]->glfwWindow());
    }

    anyOpen = false;
    for (Window* w : impl_->windows_) {
        if (w && w->isAlive()) { anyOpen = true; break; }
    }
    if (!anyOpen || impl_->shouldQuit_) return false;
    return true;
}

// ––––––––[ update ] pump one frame with auto dt
bool Application::update() {
    if (!impl_) return false;
    double now = glfwGetTime();
    float dt = static_cast<float>(now - impl_->lastTime_);
    if (impl_->lastTime_ == 0.0) dt = 0.016f;
    if (dt < 0.0f) dt = 0.0f;
    if (dt > 0.050f) dt = 0.050f;
    impl_->lastTime_ = now;
    return update(dt);
}

// ––––––––[ run ] blocking loop over all windows
void Application::run() {
    if (!impl_) return;
    impl_->shouldQuit_ = false;
    impl_->lastTime_ = glfwGetTime();
    // Initial layout for all windows (mirrors Window::run pre-layout block)
    for (Window* w : impl_->windows_) {
        if (!w || !w->isAlive()) continue;
        GLFWwindow* gw = w->glfwWindow();
        glfwSetWindowUserPointer(gw, w);
        glfwSetCursorPosCallback(gw, &Window::s_cursor_pos_cb);
        glfwSetMouseButtonCallback(gw, &Window::s_mouse_btn_cb);
        glfwSetFramebufferSizeCallback(gw, &Window::s_framebuffer_size_cb);
        glfwSetKeyCallback(gw, &Window::s_key_cb);
        glfwSetCharCallback(gw, &Window::s_char_cb);
        glfwSetScrollCallback(gw, &Window::s_scroll_cb);
        glfwSetWindowCloseCallback(gw, &Window::s_close_cb);
        w->syncSize();
        const auto [dw, dh] = w->designSize();
        for (auto& wgt : w->widgets_) {
            if (wgt->fillsParent()) wgt->fillToWindow(float(dw), float(dh));
        }
        ResizeEvent init_layout{dw, dh};
        for (auto& wgt : w->widgets_) wgt->onResize(init_layout);
    }

#if defined(FLUFFY_EMSCRIPTEN) || defined(__EMSCRIPTEN__)
    g_emscriptenApp = this;
    emscripten_set_main_loop(emscriptenMainLoop, 0, true);
#else
    while (update()) {
        // update handles poll/tick/render
    }
#endif
}

#if defined(FLUFFY_EMSCRIPTEN) || defined(__EMSCRIPTEN__)
void Application::emscriptenMainLoop() {
    if (!g_emscriptenApp || !g_emscriptenApp->impl_) return;
    if (g_emscriptenApp->impl_->shouldQuit_) {
        emscripten_cancel_main_loop();
        return;
    }
    bool anyOpen = false;
    for (Window* w : g_emscriptenApp->impl_->windows_) {
        if (w && w->isAlive()) {
            anyOpen = true;
            break;
        }
    }
    if (!anyOpen) {
        emscripten_cancel_main_loop();
        return;
    }
    g_emscriptenApp->update();
}
#endif

} // namespace fluffy
