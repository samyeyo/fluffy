// src/webview_mac.cpp
// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
// WebView — macOS overlay via WKWebView (native, no WebView2Loader)
// Semantic bridge via WKScriptMessageHandler.
// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
#ifdef FLUFFY_ENABLE_WEBVIEW
#include <fluffy/webview.hpp>
#include <fluffy/window.hpp>
#include <GLFW/glfw3native.h>
#if defined(__APPLE__)
#include <objc/runtime.h>
#include <objc/message.h>
#include <cstdio>
#include <unordered_map>
#endif

namespace fluffy {

#if defined(__APPLE__)
template<typename Ret, typename... Args>
static Ret send(void* obj, const char* selName, Args... args) {
    SEL sel = sel_registerName(selName);
    auto fn = (Ret (*)(void*, SEL, Args...))objc_msgSend;
    return fn(obj, sel, args...);
}
static void* getNSWindow(const Window& w) { return w.nativeHandle(); }

// ––––––––[ kBridgeName ] WKScriptMessageHandler name used by the JS bridge
// Pages call the platform bridge directly (window.webkit.messageHandlers.fluffy
// .postMessage) — Fluffy injects no page-side shims.
static const char kBridgeName[] = "fluffy";

// ––––––––[ bridgeOwners ] maps handler instances to their WebView
static std::unordered_map<void*, WebView*>& bridgeOwners() {
    static std::unordered_map<void*, WebView*> owners;
    return owners;
}

// ––––––––[ WebViewBridge ] ObjC script-message glue, friend of WebView
struct WebViewBridge {
    // ––––––––[ onScriptMessage ] WKScriptMessageHandler callback, forwards to WebView
    static void onScriptMessage(void* self, void*, void*, void* message) {
        auto it = bridgeOwners().find(self);
        if (it == bridgeOwners().end() || !it->second) return;
        void* body = send<void*>(message, "body");
        if (!body) return;
        if (!class_respondsToSelector(object_getClass(reinterpret_cast<id>(body)),
                                      sel_registerName("UTF8String"))) {
            body = send<void*>(body, "description");
            if (!body) return;
        }
        const char* utf8 = send<const char*>(body, "UTF8String");
        if (utf8) it->second->handleBridgeMessage(utf8);
    }
    // ––––––––[ bind ] registers a handler instance for a WebView
    static void bind(void* handler, WebView* wv) { bridgeOwners()[handler] = wv; }
    // ––––––––[ unbind ] removes a handler instance
    static void unbind(void* handler) { bridgeOwners().erase(handler); }
};
#endif

// ––––––––[ createNative ] creates WKWebView inside a clipped container NSView.
void WebView::createNative(const Window& w) {
#if defined(__APPLE__)
    if (impl_->created) return;
    void* nsWindow = getNSWindow(w);
    if (!nsWindow) return;
    void* contentView = send<void*>(nsWindow, "contentView");
    if (!contentView) return;
    Class cfgCls = objc_getClass("WKWebViewConfiguration");
    Class wkCls = objc_getClass("WKWebView");
    Class viewCls = objc_getClass("NSView");
    if (!cfgCls || !wkCls || !viewCls) return;
    void* cfg = send<void*>(send<void*>((void*)cfgCls, "alloc"), "init");
    // Script bridge: user content controller + message handler. Pages use
    // the platform-native bridge object; nothing is injected into the page.
    void* ucc = send<void*>(send<void*>((void*)objc_getClass("WKUserContentController"), "alloc"), "init");
    if (ucc) {
        send<void>(cfg, "setUserContentController:", ucc);
        Class handlerCls = objc_getClass("FluffyScriptMessageHandler");
        if (!handlerCls) {
            handlerCls = objc_allocateClassPair(objc_getClass("NSObject"), "FluffyScriptMessageHandler", 0);
            class_addMethod(handlerCls,
                            sel_registerName("userContentController:didReceiveScriptMessage:"),
                            reinterpret_cast<IMP>(WebViewBridge::onScriptMessage), "v@:@@");
            objc_registerClassPair(handlerCls);
        }
        void* handler = class_createInstance(handlerCls, 0);
        WebViewBridge::bind(handler, this);
        void* bridgeName = send<void*>((void*)objc_getClass("NSString"), "stringWithUTF8String:", kBridgeName);
        send<void>(ucc, "addScriptMessageHandler:name:", handler, bridgeName);
        impl_->bridge_handler = handler;
    }
    send<void>(ucc, "release");
    auto [winW, winH] = w.designSize();
    double fx = static_cast<double>(bounds_.x);
    double fy = static_cast<double>(winH) - static_cast<double>(bounds_.y)
              - static_cast<double>(bounds_.h);
    double fw = static_cast<double>(bounds_.w);
    double fh = static_cast<double>(bounds_.h);
    struct CGRect { double x, y, w, h; };
    CGRect frame{fx, fy, fw, fh};
    CGRect inner{0, 0, fw, fh};
    // Container that will clip with rounded corners (tracks theme)
    void* clipAlloc = send<void*>((void*)viewCls, "alloc");
    void* clipView = reinterpret_cast<void* (*)(void*, SEL, CGRect)>(objc_msgSend)(clipAlloc, sel_registerName("initWithFrame:"), frame);
    if (!clipView) return;
    // WKWebView fills container
    void* wkAlloc = send<void*>((void*)wkCls, "alloc");
    void* wk = reinterpret_cast<void* (*)(void*, SEL, CGRect, void*)>(objc_msgSend)(wkAlloc, sel_registerName("initWithFrame:configuration:"), inner, cfg);
    if (!wk) { send<void>(clipView, "release"); return; }
    // Enable layer clipping on container
    send<void>(clipView, "setWantsLayer:", static_cast<BOOL>(1));
    void* layer = send<void*>(clipView, "layer");
    if (layer) {
        float r = effectiveRadius(w.theme());
        reinterpret_cast<void (*)(void*, SEL, double)>(objc_msgSend)(layer, sel_registerName("setCornerRadius:"), static_cast<double>(r));
        reinterpret_cast<void (*)(void*, SEL, BOOL)>(objc_msgSend)(layer, sel_registerName("setMasksToBounds:"), static_cast<BOOL>(1));
    }
    send<void>(clipView, "addSubview:", wk);
    // Make wk resize with container
    const long mask = 18; // NSViewWidthSizable | NSViewHeightSizable
    reinterpret_cast<void (*)(void*, SEL, long)>(objc_msgSend)(wk, sel_registerName("setAutoresizingMask:"), mask);
    send<void>(contentView, "addSubview:", clipView);
    send<void>(wk, "setHidden:", (BOOL)0);
    impl_->native = wk;
    impl_->controller = clipView;
    impl_->created = true;
    impl_->last_bounds = bounds_;
    impl_->last_applied_radius = effectiveRadius(w.theme());
    if (impl_->has_pending_html) {
        void* nsHtml = send<void*>((void*)objc_getClass("NSString"), "stringWithUTF8String:", impl_->pending_html.c_str());
        void* baseURL = nullptr;
        if (!impl_->pending_base_url.empty()) {
            void* baseStr = send<void*>((void*)objc_getClass("NSString"), "stringWithUTF8String:", impl_->pending_base_url.c_str());
            baseURL = send<void*>((void*)objc_getClass("NSURL"), "URLWithString:", baseStr);
            if (!baseURL) baseURL = send<void*>((void*)objc_getClass("NSURL"), "fileURLWithPath:", baseStr);
        }
        send<void>(wk, "loadHTMLString:baseURL:", nsHtml, baseURL);
    } else if (!url_.empty()) {
        void* nsStr = send<void*>((void*)objc_getClass("NSString"), "stringWithUTF8String:", url_.c_str());
        void* nsURL = send<void*>((void*)objc_getClass("NSURL"), "URLWithString:", nsStr);
        if (nsURL) {
            void* req = send<void*>(send<void*>((void*)objc_getClass("NSURLRequest"), "alloc"), "initWithURL:", nsURL);
            send<void>(wk, "loadRequest:", req);
        }
    }
    (void)cfg;
#else
    (void)w;
    impl_->created = true;
    impl_->native = nullptr;
#endif
}

// ––––––––[ destroyNative ] removes WKWebView and container.
void WebView::destroyNative() {
#if defined(__APPLE__)
    if (impl_) {
        if (impl_->bridge_handler) {
            if (impl_->native) {
                void* cfg = send<void*>(impl_->native, "configuration");
                void* ucc = cfg ? send<void*>(cfg, "userContentController") : nullptr;
                if (ucc) {
                    void* bridgeName = send<void*>((void*)objc_getClass("NSString"), "stringWithUTF8String:", kBridgeName);
                    send<void>(ucc, "removeScriptMessageHandlerForName:", bridgeName);
                }
            }
            WebViewBridge::unbind(impl_->bridge_handler);
            send<void>(impl_->bridge_handler, "release");
            impl_->bridge_handler = nullptr;
        }
        if (impl_->controller) {
            send<void>(impl_->controller, "removeFromSuperview");
            // Release container (it retains wk)
            send<void>(impl_->controller, "release");
            impl_->controller = nullptr;
        } else if (impl_->native) {
            send<void>(impl_->native, "removeFromSuperview");
        }
        impl_->native = nullptr;
    }
#endif
    if (impl_) {
        impl_->created = false;
        impl_->last_applied_radius = -999.0f;
    }
}

// ––––––––[ syncBounds ] updates container frame (WKWebView follows via autoresizing).
void WebView::syncBounds(const Window& w) {
#if defined(__APPLE__)
    if (!impl_ || !impl_->native) return;
    if (impl_->last_bounds.x == bounds_.x && impl_->last_bounds.y == bounds_.y &&
        impl_->last_bounds.w == bounds_.w && impl_->last_bounds.h == bounds_.h) return;
    auto [winW2, winH2] = w.designSize();
    (void)winW2;
    double fx = static_cast<double>(bounds_.x);
    double fy = static_cast<double>(winH2) - static_cast<double>(bounds_.y)
              - static_cast<double>(bounds_.h);
    double fw = static_cast<double>(bounds_.w);
    double fh = static_cast<double>(bounds_.h);
    struct CGRect2 { double x, y, w, h; };
    CGRect2 frame{fx, fy, fw, fh};
    CGRect2 inner{0, 0, fw, fh};
    void* target = impl_->controller ? impl_->controller : impl_->native;
    reinterpret_cast<void (*)(void*, SEL, CGRect2)>(objc_msgSend)(target, sel_registerName("setFrame:"), frame);
    // Keep inner WKWebView filling container
    if (impl_->controller && impl_->native) {
        reinterpret_cast<void (*)(void*, SEL, CGRect2)>(objc_msgSend)(impl_->native, sel_registerName("setFrame:"), inner);
    }
    impl_->last_bounds = bounds_;
    // Re-apply radius if scale changed (frame change may affect backing)
    applyCornerRadius(w);
#else
    (void)w;
    if (impl_) impl_->last_bounds = bounds_;
#endif
}

// ––––––––[ applyCornerRadius ] updates container layer corner radius.
void WebView::applyCornerRadius(const Window& w) {
#if defined(__APPLE__)
    if (!impl_ || !impl_->native) return;
    float r = effectiveRadius(w.theme());
    if (r == impl_->last_applied_radius) return;
    impl_->last_applied_radius = r;
    void* clip = impl_->controller;
    if (!clip) clip = impl_->native;
    // Ensure layer exists
    send<void>(clip, "setWantsLayer:", static_cast<BOOL>(1));
    void* layer = send<void*>(clip, "layer");
    if (!layer) return;
    reinterpret_cast<void (*)(void*, SEL, double)>(objc_msgSend)(layer, sel_registerName("setCornerRadius:"), static_cast<double>(r));
    reinterpret_cast<void (*)(void*, SEL, BOOL)>(objc_msgSend)(layer, sel_registerName("setMasksToBounds:"), static_cast<BOOL>(1));
#else
    (void)w;
    if (impl_) impl_->last_applied_radius = effectiveRadius(w.theme());
#endif
}

// ––––––––[ navigateNative ] loads a URL in WKWebView.
void WebView::navigateNative(const std::string& url) {
#if defined(__APPLE__)
    if (!impl_ || !impl_->native) return;
    void* wk = impl_->native;
    if (url.rfind("file://", 0) == 0) {
        void* nsStr = send<void*>((void*)objc_getClass("NSString"), "stringWithUTF8String:", url.c_str());
        void* nsURL = send<void*>((void*)objc_getClass("NSURL"), "URLWithString:", nsStr);
        if (nsURL) {
            bool isDir = !url.empty() && url.back() == '/';
            void* allowURL = nsURL;
            if (!isDir) {
                allowURL = send<void*>(nsURL, "URLByDeletingLastPathComponent");
                if (!allowURL) allowURL = nsURL;
            }
            send<void>(wk, "loadFileURL:allowingReadAccessToURL:", nsURL, allowURL);
            return;
        }
    }
    void* nsStr = send<void*>((void*)objc_getClass("NSString"), "stringWithUTF8String:", url.c_str());
    void* nsURL = send<void*>((void*)objc_getClass("NSURL"), "URLWithString:", nsStr);
    if (nsURL) {
        void* req = send<void*>(send<void*>((void*)objc_getClass("NSURLRequest"), "alloc"), "initWithURL:", nsURL);
        send<void>(wk, "loadRequest:", req);
    }
#else
    (void)url;
#endif
}

// ––––––––[ setHtmlNative ] loads markup with an optional base URL.
bool WebView::setHtmlNative(const std::string& html, const std::string& baseFileUrl) {
#if defined(__APPLE__)
    if (!impl_ || !impl_->native) return false;
    void* nsHtml = send<void*>((void*)objc_getClass("NSString"), "stringWithUTF8String:", html.c_str());
    void* baseURL = nullptr;
    if (!baseFileUrl.empty()) {
        void* baseStr = send<void*>((void*)objc_getClass("NSString"), "stringWithUTF8String:", baseFileUrl.c_str());
        baseURL = send<void*>((void*)objc_getClass("NSURL"), "URLWithString:", baseStr);
        if (!baseURL) baseURL = send<void*>((void*)objc_getClass("NSURL"), "fileURLWithPath:", baseStr);
    }
    send<void>(impl_->native, "loadHTMLString:baseURL:", nsHtml, baseURL);
    return true;
#else
    (void)html;
    (void)baseFileUrl;
    return false;
#endif
}

// ––––––––[ evalJSNative ] evaluates JS and returns the result through the bridge.
bool WebView::evalJSNative(const std::string& js, int id) {
#if defined(__APPLE__)
    if (!impl_ || !impl_->native) return false;
    const std::string tag = "__fluffy_eval:" + std::to_string(id);
    const std::string wrapped =
        "(function(){try{var r=String((" + js + "));"
        "window.webkit.messageHandlers.fluffy.postMessage(\"" + tag + ":r:\"+r);}"
        "catch(e){window.webkit.messageHandlers.fluffy.postMessage(\"" + tag + ":e:\"+String(e));}})()";
    void* nsStr = send<void*>((void*)objc_getClass("NSString"), "stringWithUTF8String:", wrapped.c_str());
    send<void>(impl_->native, "evaluateJavaScript:completionHandler:", nsStr, nullptr);
    return true;
#else
    (void)js;
    (void)id;
    return false;
#endif
}

void WebView::tickNative() {}

}  // namespace fluffy

// Provide non-Apple stubs for other platforms when this file is compiled on Linux/Win (should not happen due to CMake, but guard)
#if !defined(__APPLE__)
namespace fluffy {
void WebView::createNative(const Window&) {}
void WebView::destroyNative() {}
void WebView::syncBounds(const Window&) {}
}
#endif

#else
// FLUFFY_ENABLE_WEBVIEW not defined — no-op (handled in webview.cpp)
#endif
