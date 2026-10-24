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

namespace fluffy {

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
        int lw, lh, fbw, fbh;
        glfwGetWindowSize(gw, &lw, &lh);
        glfwGetFramebufferSize(gw, &fbw, &fbh);
        for (auto& wgt : w->widgets_) {
            if (wgt->fillsParent()) wgt->setBounds(Rect(0.0f, 0.0f, float(lw), float(lh)));
        }
        ResizeEvent init_layout{fbw, fbh};
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
