// include/fluffy/webview.hpp
// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
// WebView — native overlay (WKWebView / WebKitGTK / WebView2)
// PostMessage bridge + JS evaluation; navigation/title callbacks are reserved.
// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
#pragma once
#include <fluffy/widget.hpp>
#include <functional>
#include <memory>
#include <string>
#include <vector>
namespace fluffy {
struct WebViewBridge;
class WebView final : public Widget {
public:
    using MessageHandler = std::function<void(const std::string& json, WebView& self)>;
    using NavigationHandler = std::function<bool(const std::string& url)>;
    using TitleHandler = std::function<void(const std::string& title)>;

    explicit WebView(Rect bounds = Rect{}, std::string url = "https://example.com");
    ~WebView() override;

    WebView(const WebView&) = delete;
    WebView& operator=(const WebView&) = delete;

    // ––––––––[ navigate ] loads a URL.
    void navigate(const std::string& url);

    // ––––––––[ setHtml ] loads raw HTML.
    void setHtml(const std::string& html);
    // ––––––––[ setHtml ] loads HTML with file base URL for local assets / CORS.
    void setHtml(const std::string& html, const std::string& baseFileUrl);

    // ––––––––[ evalJS ] evaluates JavaScript.
    void evalJS(const std::string& js, std::function<void(const std::string&)> cb = nullptr);

    // ––––––––[ setOnMessage ] semantic postMessage handler.
    void setOnMessage(MessageHandler h) { on_message_ = std::move(h); }

    // ––––––––[ setOnNavigation ] return false to block.
    void setOnNavigation(NavigationHandler h) { on_navigation_ = std::move(h); }

    // ––––––––[ setOnTitle ] title change callback.
    void setOnTitle(TitleHandler h) { on_title_ = std::move(h); }

    void draw(NVGcontext* vg, const Theme& theme, const Window& w) override;
    EventResult handleEvent(const Event& e, const Window& w) override;
    void tick(float dt, const Theme& theme) noexcept override;
    bool isFocusable() const noexcept override { return true; }
    float naturalWidth(const Theme& theme, const Window& w) const override;
    float naturalHeight(const Theme& theme, const Window& w) const override;
    void setProperty(std::string_view name, const YamlNode& value) override;

    // ––––––––[ corner radius ] -1 tracks Theme::metrics.corner_radius
    void setCornerRadius(float r) noexcept;
    float cornerRadius() const noexcept { return cornerRadius_; }
    float effectiveRadius(const Theme& theme) const noexcept;

private:
    // ––––––––[ WebViewBridge ] platform bridge glue — forwards script messages
    friend struct WebViewBridge;

    // ––––––––[ handleBridgeMessage ] routes bridge payloads to eval callbacks or on_message_
    void handleBridgeMessage(const std::string& payload);

    // ––––––––[ Impl ] native handles, bridge state and pending eval callbacks
    struct Impl {
        // ––––––––[ native ] WKWebView* (mac), HWND (win), GtkWidget* (linux)
        void* native = nullptr;
        // ––––––––[ controller ] mac: clip NSView*, win: HWND, linux: GtkWidget*
        void* controller = nullptr;
        // ––––––––[ bridge_handler ] mac: WKScriptMessageHandler instance
        void* bridge_handler = nullptr;
        // ––––––––[ created ] true once the native view exists
        bool created = false;
        // ––––––––[ last_bounds ] bounds last applied to the native view
        Rect last_bounds{};
        // ––––––––[ last_applied_radius ] corner radius last applied to the layer
        float last_applied_radius = -999.0f;
        // ––––––––[ pending_evals ] evalJS callbacks awaiting bridge results
        std::vector<std::function<void(const std::string&)>> pending_evals;
        // ––––––––[ pending_html ] markup queued before the native view exists
        std::string pending_html;
        // ––––––––[ pending_base_url ] base URL for pending_html
        std::string pending_base_url;
        // ––––––––[ has_pending_html ] true when pending_html should be loaded
        bool has_pending_html = false;
    };
    std::unique_ptr<Impl> impl_;
    std::string url_;
    MessageHandler on_message_;
    NavigationHandler on_navigation_;
    TitleHandler on_title_;
    float cornerRadius_ = -1.0f; // -1 = track theme

    void createNative(const Window& w);
    void destroyNative();
    void syncBounds(const Window& w);
    void applyCornerRadius(const Window& w);
    void navigateNative(const std::string& url);
    bool setHtmlNative(const std::string& html, const std::string& baseFileUrl);
    bool evalJSNative(const std::string& js, int id);
    void tickNative();
};
}  // namespace fluffy
