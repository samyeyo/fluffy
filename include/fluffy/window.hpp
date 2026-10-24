// include/fluffy/window.hpp

// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

// Window — top-level façade: GLFW, OpenGL, NanoVG, events, theme, fonts.

#pragma once
#include <fluffy/container.hpp>
#include <fluffy/dispatcher.hpp>
#include <fluffy/error.hpp>
#include <fluffy/event.hpp>
#include <fluffy/inline.hpp>
#include <fluffy/theme.hpp>
#include <fluffy/widget.hpp>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
struct GLFWwindow;
struct NVGcontext;
namespace fluffy {
class Application;

// ––––––––[ WindowButtons ] bitmask of native title-bar window buttons
enum class WindowButtons : unsigned {
    None     = 0,
    Close    = 1u << 0,
    Minimize = 1u << 1,
    Maximize = 1u << 2,
    All      = Close | Minimize | Maximize,
};

// ––––––––[ operator| ] combines window button flags
constexpr WindowButtons operator|(WindowButtons a, WindowButtons b) noexcept {
    return static_cast<WindowButtons>(static_cast<unsigned>(a) |
                                      static_cast<unsigned>(b));
}

// ––––––––[ operator& ] intersects window button flags
constexpr WindowButtons operator&(WindowButtons a, WindowButtons b) noexcept {
    return static_cast<WindowButtons>(static_cast<unsigned>(a) &
                                      static_cast<unsigned>(b));
}

// ––––––––[ CursorShape ] platform-independent mouse cursor shapes
enum class CursorShape { Arrow, IBeam, Hand, Crosshair, HResize, VResize };

class Window {
public:
    struct Config {
        std::string title  = "Fluffy";
        int  width          = 800;
        int  height         = 600;
        int  min_width      = 0;   // 0 = no minimum
        int  min_height     = 0;   // 0 = no minimum
        bool resizable      = true;
        bool vsync          = true;
        bool visible        = true;
        bool decorated      = true;
        Theme theme         = Theme::defaultLight();
    };
    [[nodiscard]] static Result<Window> create(const Config& cfg);
    // ––––––––[ open ] simple creation, check `if (!win)` instead of Result
    static Window open(std::string title, int width, int height);
    ~Window();
    Window(const Window&)            = delete;
    Window& operator=(const Window&) = delete;
    Window(Window&&)                 noexcept;
    Window& operator=(Window&&)      noexcept;
    void run();
    void close()    noexcept { requestClose(); }
    void requestClose() noexcept;
    Container& verticalLayout();
    Container& horizontalLayout();
// ––––––––[ gridLayout ] root container with grid layout of given columns
    Container& gridLayout(int cols = 2);
    enum class DialogButtons { Ok, OkCancel, YesNo, YesNoCancel };
    // ––––––––[ DialogChoice ] typed dialog answer returned by showDialog
    enum class DialogChoice { Dismissed = -1, First = 0, Second = 1, Third = 2 };
    // ––––––––[ showDialog ] preset buttons, returns the chosen index
    DialogChoice showDialog(const std::string& title,
                            const std::string& message,
                            DialogButtons buttons = DialogButtons::Ok);
    // ––––––––[ showDialog ] custom plain labels, returns the chosen index
    DialogChoice showDialog(const std::string& title,
                            const std::string& message,
                            const std::vector<std::string>& buttonLabels);
    // ––––––––[ showDialog ] inline labels, returns the chosen index
    DialogChoice showDialog(const std::string& title,
                            const std::string& message,
                            const std::vector<Inline>& buttonContent);
    // ––––––––[ showDialog ] inline message and labels, returns the chosen index
    DialogChoice showDialog(const std::string& title,
                            const Inline& message,
                            const std::vector<Inline>& buttonContent);
    EventDispatcher& dispatcher() noexcept { return dispatcher_; }
    NoneResult            load(const std::string& filepath);
    NoneResult            loadFromString(std::string_view yaml_content);
    // ––––––––[ add ] creates a widget owned by the window
    template <typename W, typename... Args>
    W& add(Args&&... args) {
        auto owned = std::make_unique<W>(std::forward<Args>(args)...);
        return static_cast<W&>(*adopt(std::move(owned)));
    }
    // ––––––––[ operator bool ] true while the window is alive and openable
    explicit operator bool() const noexcept { return isAlive(); }
    Widget* adopt(std::unique_ptr<Widget> w);
    std::size_t widgetCount() const noexcept { return widgets_.size(); }
    Widget* rootWidget(std::size_t i = 0) const noexcept {
        return i < widgets_.size() ? widgets_[i].get() : nullptr;
    }
    Theme&       theme()       noexcept { return theme_; }
    const Theme& theme() const noexcept { return theme_; }
    void         setTheme(const Theme& t) noexcept;
    using ResizeHandler = std::function<void(int, int)>;
    void setOnResize(ResizeHandler h) { onResize_ = std::move(h); }
    using EventHandler = std::function<EventResult(const Event&, Window&)>;
    void setEventHandler(EventHandler h) { eventHandler_ = std::move(h); }
    void clearEventHandler() noexcept { eventHandler_ = {}; }
    // ––––––––[ onKey ] simple key callback without touching raw events
    void onKey(std::function<bool(int key, int mods)> h);
    // ––––––––[ onClose ] simple close interceptor, return true to keep open
    void onClose(std::function<bool()> h);
    // ––––––––[ setEnabledButtons ] limits which title-bar buttons are active
    void setEnabledButtons(WindowButtons flags) noexcept;
    WindowButtons enabledButtons() const noexcept { return enabledButtons_; }
    void cursorPosition(float& x, float& y) const noexcept {
        x = cursor_x_; y = cursor_y_;
    }
    void setCursorPosition(float x, float y) noexcept { cursor_x_ = x; cursor_y_ = y; }
    std::pair<int,int> designSize() const noexcept;
    void registerFontMemory(const std::string& name,
                              const void* data,
                              std::size_t size);
    bool loadSystemFont(const std::string& family_name);
    bool prepareFont(NVGcontext* vg,
                      const std::string& spec,
                      std::string& out_handle) const;
    NVGcontext* nvgContext() const noexcept;
    // ––––––––[ setCursorShape ] sets the mouse cursor for this window
    void setCursorShape(CursorShape shape);
    void requestFocus(Widget* w) noexcept;
    void clearFocus() noexcept;
    // ––––––––[ focusNext ] moves keyboard focus to the next focusable widget
    void focusNext();
    // ––––––––[ focusPrevious ] moves keyboard focus to the previous focusable widget
    void focusPrevious();
    const char* getClipboardString() const;
    void        setClipboardString(const char* str);
    // ––––––––[ nativeHandle ] returns NSWindow* (mac), HWND (win), Window (x11) or nullptr.
     void* nativeHandle() const noexcept;
    // ––––[ popup overlay ] show/hide a popup widget rendered on top of everything.
    void showPopup(std::unique_ptr<Widget> popup);
    void closePopup();
    void closePopupDeferred() noexcept { popupPendingClose_ = true; }
    bool hasPopup() const noexcept { return popupPtr_ != nullptr; }
    // ––––––––[ popup ] returns current popup widget or nullptr
    Widget* popup() const noexcept { return popupPtr_; }
    // ––––––––[ renderFrame ] renders one frame and presents it
    void renderFrame();
    // ––––––––[ saveScreenshot ] saves current FBO contents as PNG
    bool saveScreenshot(const std::string& path) const;
    // ––––––––[ glfwWindow ] returns GLFWwindow* for Application loop
    GLFWwindow* glfwWindow() const noexcept;
    // ––––––––[ isAlive ] true if window not closed
    bool isAlive() const noexcept;
    // ––––––––[ syncSize ] updates logical/fb size from GLFW
    void syncSize();
    // ––––––––[ doHoverTick ] updates hover, tooltip and tick for a frame
    void doHoverTick(float dt);
    friend class Application;
private:
    Window() = default;
    // ––––––––[ focusStep ] walks the focus chain; dir +1 forward, -1 backward
    void focusStep(int dir);
    void applyWindowButtons();
    void applyWindowAppearance();
    void resizeFbo();
    struct DialogSpec {
        std::string label;
        Inline      inline_content;
    };
    std::string showDialogImpl(const std::string& title,
                                 const Inline& message,
                                 const std::vector<DialogSpec>& specs);
    static void s_cursor_pos_cb    (::GLFWwindow* w, double x, double y);
    static void s_mouse_btn_cb     (::GLFWwindow* w, int button, int action, int mods);
    static void s_framebuffer_size_cb(::GLFWwindow* w, int w2, int h2);
    static void s_key_cb           (::GLFWwindow* w, int key, int scancode, int action, int mods);
    static void s_char_cb          (::GLFWwindow* w, unsigned int codepoint);
    static void s_scroll_cb        (::GLFWwindow* w, double x, double y);
    static void s_close_cb         (::GLFWwindow* w);
    static void dialogFrame(Window& parent, Window& popup);
    static void flushDeferredPopupClose(Window* win) noexcept;
#if defined(FLUFFY_EMSCRIPTEN) || defined(__EMSCRIPTEN__)
    void emscriptenFrame();
    static void emscriptenMainLoop();
    static Window* g_emscriptenWindow;
#endif
    struct Impl;
    std::unique_ptr<Impl> impl_;
    EventDispatcher                     dispatcher_;
    Theme                               theme_{};
    std::vector<std::unique_ptr<Widget>> widgets_;
    ResizeHandler                       onResize_;
    EventHandler                        eventHandler_;
    bool                                should_close_ = false;
    float                               cursor_x_ = 0.0f;
    float                               cursor_y_ = 0.0f;
    std::vector<Event>                  pending_events_;
    double                              last_time_ = 0.0;
    WindowButtons enabledButtons_ = WindowButtons::All;
    Container*                          root_container_  = nullptr;
    Widget*                             focusWidget_     = nullptr;
    Widget*                             popupPtr_        = nullptr;
    bool                                popupPendingClose_ = false;
    std::vector<Widget*>                prevPopups_;
    Widget*                             tooltip_hover_widget_ = nullptr;
    double                              tooltip_hover_start_ = 0.0;
};
}  

