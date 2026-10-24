// include/fluffy/application.hpp
// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
// Application — static lazy singleton: owns GLFW lifetime, window registry,
// global event loop and manual update pumping.
//
// MIT License - Copyright (c) 2026 - Samir Tine
// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

#pragma once
#include <cstddef>
#include <optional>

namespace fluffy {
class Window;

// ––––––––[ Application ] static lazy singleton, one per process
class Application {
public:
    // ––––––––[ get ] lazy Meyer's singleton
    static Application& get();

    // ––––––––[ tryGet ] non-creating accessor, nullptr if never created
    static Application* tryGet() noexcept;

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
    Application(Application&&) = delete;
    Application& operator=(Application&&) = delete;

    // ––––––––[ windowCount ] number of live registered windows
    std::size_t windowCount() const noexcept;

    // ––––––––[ window ] returns nth registered window or nullptr
    Window* window(std::size_t i) const noexcept;

    // ––––––––[ run ] blocking global loop over all windows
    void run();

    // ––––––––[ update ] pumps one frame for all windows
    bool update();

    // ––––––––[ update ] pumps one frame with explicit delta seconds
    bool update(float delaySeconds);

    // ––––––––[ quit ] requests all windows to close and exits run()
    void quit() noexcept;

    // ––––––––[ shouldQuit ] true after quit() called
    bool shouldQuit() const noexcept;

private:
    Application();
    ~Application();

    // ––––––––[ registerWindow ] called by Window after successful create
    void registerWindow(Window* w);

    // ––––––––[ unregisterWindow ] called by Window dtor / move
    void unregisterWindow(Window* w) noexcept;

    friend class Window;

    struct Impl;
    Impl* impl_ = nullptr;

#if defined(FLUFFY_EMSCRIPTEN) || defined(__EMSCRIPTEN__)
    static void emscriptenMainLoop();
#endif
};
} // namespace fluffy
