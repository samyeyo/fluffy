// examples/webview/main.cpp
// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
// WebView overlay example — native WebView (WebKit on macOS/Linux, WebView2 on Windows).
// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
#include <fluffy/button.hpp>
#include <fluffy/label.hpp>
#include <fluffy/theme.hpp>
#include <fluffy/webview.hpp>
#include <fluffy/window.hpp>

int main() {
    using namespace fluffy;

    Window win = Window::open("Fluffy — WebView (overlay)", 900, 600);
    if (!win) return 1;
    win.setTheme(Theme::defaultLight());

    auto& title = win.add<Label>(
        Rect(16, 12, 868, 28),
        "Fluffy WebView — native overlay (WebKit / WebView2)",
        Label::Align::Left, Label::Align::Center);
    title.setTextSize(18.0f);

    // WebView occupies central area — native overlay, 60fps
    auto& wv = win.add<WebView>(
        Rect(16, 48, 868, 460),
        "https://google.com");
    wv.setOnMessage([&title](const std::string& json, WebView&) {
        title.setText("Message: " + json);
    });
    wv.setOnNavigation([](const std::string& url) {
        // Allow all, log
        (void)url;
        return true;
    });

    // The page-side bridge call is platform-native: WKWebView on macOS,
    // WebKitGTK on Linux, WebView2 on Windows. Fluffy injects no shims.
#if defined(_WIN32)
    const std::string post_msg = "window.chrome.webview.postMessage";
#else
    const std::string post_msg = "window.webkit.messageHandlers.fluffy.postMessage";
#endif

    auto& nav = win.add<Button>(
        Rect(16, 520, 140, 32),
        "Navigate");
    nav.setOnClick([&wv, &title]() {
        wv.navigate("https://www.google.com");
        title.setText("Navigated to https://www.google.com");
    });

    auto& html = win.add<Button>(
        Rect(168, 520, 140, 32),
        "Load HTML");
    html.setOnClick([&wv, &title, post_msg]() {
        wv.setHtml("<html><body style='font-family:Inter,system-ui;padding:24px'>"
                    "<h2>Hello from Fluffy</h2>"
                    "<p>This is a <b>native</b> WebView overlay."
                    "<br><button onclick=\"" + post_msg +
                    "(JSON.stringify({type:'buy',id:42}))\">PostMessage</button>"
                    "</p></body></html>");
        title.setText("Loaded HTML — Hello from Fluffy");
    });

    auto& js = win.add<Button>(
        Rect(320, 520, 140, 32),
        "Eval JS");
    js.setOnClick([&wv, &title]() {
        wv.evalJS("document.title", [&title](const std::string& res) {
            title.setText("Title: " + res);
        });
    });

    win.run();
    return 0;
}
