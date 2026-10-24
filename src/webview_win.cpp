// src/webview_win.cpp
// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
// WebView — Windows overlay via WebView2 (built-in loader, no WebView2Loader.dll)
// Semantic bridge via WebMessageReceived / ExecuteScript.
// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
#ifdef FLUFFY_ENABLE_WEBVIEW
#include <fluffy/webview.hpp>
#include <fluffy/window.hpp>

#if defined(_WIN32)
#include <windows.h>
#include <wrl.h>
#include <cmath>
#include <memory>
#include <unordered_map>
#if defined(__has_include)
#  if __has_include(<webview/webview.h>)
#    include <webview/webview.h>
#    define FLUFFY_HAS_WEBVIEW2_LOADER 1
#  endif
#endif
#endif

namespace fluffy {

#if defined(_WIN32) && defined(FLUFFY_HAS_WEBVIEW2_LOADER)

using Microsoft::WRL::Callback;
using Microsoft::WRL::ComPtr;

namespace {

std::wstring widen(const std::string& s) {
    if (s.empty()) return {};
    int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
    std::wstring out(static_cast<std::size_t>(n), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), out.data(), n);
    return out;
}

std::string narrow(const std::wstring& s) {
    if (s.empty()) return {};
    int n = WideCharToMultiByte(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0, nullptr, nullptr);
    std::string out(static_cast<std::size_t>(n), '\0');
    WideCharToMultiByte(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), out.data(), n, nullptr, nullptr);
    return out;
}

struct WinState {
    WebView* self = nullptr;
    HWND host = nullptr;
    ComPtr<ICoreWebView2Controller> controller;
    ComPtr<ICoreWebView2> webview;
    EventRegistrationToken message_token{};
    bool has_message_token = false;
    bool com_initialized = false;

    ~WinState() {
        if (has_message_token && webview) webview->remove_WebMessageReceived(message_token);
        if (controller) controller->Close();
        webview.Reset();
        controller.Reset();
        if (com_initialized) CoUninitialize();
    }
};

std::unordered_map<WebView*, std::shared_ptr<WinState>>& states() {
    static std::unordered_map<WebView*, std::shared_ptr<WinState>> map;
    return map;
}

float dpiScale(HWND hwnd) {
    using get_dpi_fn = UINT(WINAPI*)(HWND);
    static auto get_dpi = reinterpret_cast<get_dpi_fn>(
        GetProcAddress(GetModuleHandleW(L"user32.dll"), "GetDpiForWindow"));
    UINT dpi = get_dpi ? get_dpi(hwnd) : 96;
    return dpi ? static_cast<float>(dpi) / 96.0f : 1.0f;
}

void sizeHost(WinState& st, float x, float y, float w, float h) {
    if (!st.host) return;
    HWND parent = GetParent(st.host);
    float scale = dpiScale(parent);
    int px = static_cast<int>(std::lround(x * scale));
    int py = static_cast<int>(std::lround(y * scale));
    int pw = static_cast<int>(std::lround(w * scale));
    int ph = static_cast<int>(std::lround(h * scale));
    SetWindowPos(st.host, HWND_TOP, px, py, pw, ph, SWP_NOACTIVATE);
    if (st.controller) {
        RECT r{0, 0, pw, ph};
        st.controller->put_Bounds(r);
    }
}

}  // namespace

// ––––––––[ createNative ] creates the WebView2 controller inside a child host HWND.
void WebView::createNative(const Window& w) {
    if (impl_->created) return;
    HWND parent = static_cast<HWND>(w.nativeHandle());
    if (!parent) return;

    auto st = std::make_shared<WinState>();
    st->self = this;
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (SUCCEEDED(hr)) {
        st->com_initialized = true;
    } else if (hr != RPC_E_CHANGED_MODE) {
        return;
    }

    st->host = CreateWindowExW(0, L"STATIC", nullptr, WS_CHILD | WS_CLIPCHILDREN,
                               0, 0, 1, 1, parent, nullptr,
                               GetModuleHandleW(nullptr), nullptr);
    if (!st->host) {
        if (st->com_initialized) {
            CoUninitialize();
            st->com_initialized = false;
        }
        return;
    }

    states()[this] = st;
    impl_->created = true;
    impl_->controller = st->host;

    auto envCompleted = Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
        [st](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {
            if (FAILED(result) || !env) return result;
            auto controllerCompleted = Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                [st](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT {
                    if (FAILED(result) || !controller) {
                        if (controller) controller->Release();
                        return result;
                    }
                    st->controller.Attach(controller);
                    st->controller->get_CoreWebView2(st->webview.GetAddressOf());
                    if (!st->webview) return E_FAIL;

                    auto* self = st->self;
                    if (!self) return S_OK;
                    self->impl_->native = st->webview.Get();
                    self->impl_->controller = st->host;

                    st->webview->add_WebMessageReceived(
                        Callback<ICoreWebView2WebMessageReceivedEventHandler>(
                            [st](ICoreWebView2*, ICoreWebView2WebMessageReceivedEventArgs* args) -> HRESULT {
                                auto* self = st->self;
                                if (!self) return S_OK;
                                std::wstring payload;
                                LPWSTR text = nullptr;
                                if (SUCCEEDED(args->TryGetWebMessageAsString(&text)) && text) {
                                    payload.assign(text);
                                    CoTaskMemFree(text);
                                } else {
                                    LPWSTR json = nullptr;
                                    if (SUCCEEDED(args->get_WebMessageAsJson(&json)) && json) {
                                        payload.assign(json);
                                        CoTaskMemFree(json);
                                    }
                                }
                                if (!payload.empty()) self->handleBridgeMessage(narrow(payload));
                                return S_OK;
                            }).Get(),
                        &st->message_token);
                    st->has_message_token = true;

                    if (self->window_) self->syncBounds(*self->window_);
                    ShowWindow(st->host, SW_SHOWNA);
                    st->controller->put_IsVisible(TRUE);

                    if (self->impl_->has_pending_html) {
                        const auto html = widen(self->impl_->pending_html);
                        st->webview->NavigateToString(html.c_str());
                    } else if (!self->url_.empty()) {
                        const auto url = widen(self->url_);
                        st->webview->Navigate(url.c_str());
                    }
                    return S_OK;
                });
            return env->CreateCoreWebView2Controller(st->host, controllerCompleted.Get());
        });

    webview::detail::mswebview2::loader loader;
    hr = loader.create_environment_with_options(nullptr, nullptr, nullptr, envCompleted.Get());
    if (FAILED(hr)) {
        states().erase(this);
        st->self = nullptr;
        if (st->host) DestroyWindow(st->host);
        impl_->created = false;
        impl_->controller = nullptr;
    }
}

// ––––––––[ destroyNative ] closes WebView2 and the host HWND.
void WebView::destroyNative() {
    auto it = states().find(this);
    if (it != states().end()) {
        auto st = it->second;
        states().erase(it);
        st->self = nullptr;
        if (st->has_message_token && st->webview) {
            st->webview->remove_WebMessageReceived(st->message_token);
            st->has_message_token = false;
        }
        if (st->controller) st->controller->Close();
        st->webview.Reset();
        st->controller.Reset();
        if (st->host) {
            DestroyWindow(st->host);
            st->host = nullptr;
        }
    }
    impl_->native = nullptr;
    impl_->controller = nullptr;
    impl_->created = false;
    impl_->last_applied_radius = -999.0f;
}

// ––––––––[ syncBounds ] moves and resizes the child HWND.
void WebView::syncBounds(const Window& w) {
    auto it = states().find(this);
    if (it == states().end()) {
        impl_->last_bounds = bounds_;
        applyCornerRadius(w);
        return;
    }
    const bool changed = impl_->last_bounds.x != bounds_.x ||
                         impl_->last_bounds.y != bounds_.y ||
                         impl_->last_bounds.w != bounds_.w ||
                         impl_->last_bounds.h != bounds_.h;
    sizeHost(*it->second, bounds_.x, bounds_.y, bounds_.w, bounds_.h);
    impl_->last_bounds = bounds_;
    if (changed) applyCornerRadius(w);
}

// ––––––––[ applyCornerRadius ] clips the host HWND with a rounded region.
void WebView::applyCornerRadius(const Window& w) {
    auto it = states().find(this);
    if (it == states().end()) return;
    auto& st = *it->second;
    if (!st.host) return;

    float r = effectiveRadius(w.theme());
    impl_->last_applied_radius = r;

    int iw = static_cast<int>(bounds_.w);
    int ih = static_cast<int>(bounds_.h);
    if (iw <= 0 || ih <= 0 || r <= 0.5f) {
        SetWindowRgn(st.host, nullptr, TRUE);
        return;
    }
    float scale = dpiScale(GetParent(st.host));
    int rr = static_cast<int>(std::lround(r * scale));
    int pw = static_cast<int>(std::lround(bounds_.w * scale));
    int ph = static_cast<int>(std::lround(bounds_.h * scale));
    HRGN region = CreateRoundRectRgn(0, 0, pw + 1, ph + 1, rr * 2, rr * 2);
    if (region) SetWindowRgn(st.host, region, TRUE);
}

// ––––––––[ navigateNative ] navigates WebView2.
void WebView::navigateNative(const std::string& url) {
    auto it = states().find(this);
    if (it == states().end() || !it->second->webview) return;
    it->second->webview->Navigate(widen(url).c_str());
}

// ––––––––[ setHtmlNative ] loads markup through WebView2.
bool WebView::setHtmlNative(const std::string& html, const std::string&) {
    auto it = states().find(this);
    if (it == states().end() || !it->second->webview) return false;
    it->second->webview->NavigateToString(widen(html).c_str());
    return true;
}

// ––––––––[ evalJSNative ] evaluates JS and posts the result through WebMessageReceived.
bool WebView::evalJSNative(const std::string& js, int id) {
    auto it = states().find(this);
    if (it == states().end() || !it->second->webview) return false;
    const std::string tag = "__fluffy_eval:" + std::to_string(id);
    const std::string wrapped =
        "(function(){try{var r=String((" + js + "));"
        "window.chrome.webview.postMessage(\"" + tag + ":r:\"+r);}"
        "catch(e){window.chrome.webview.postMessage(\"" + tag + ":e:\"+String(e));}})()";
    it->second->webview->ExecuteScript(widen(wrapped).c_str(), nullptr);
    return true;
}

void WebView::tickNative() {}

#else

void WebView::createNative(const Window&) { impl_->created = true; }
void WebView::destroyNative() {
    impl_->native = nullptr;
    impl_->controller = nullptr;
    impl_->created = false;
}
void WebView::syncBounds(const Window& w) {
    impl_->last_bounds = bounds_;
    applyCornerRadius(w);
}
void WebView::applyCornerRadius(const Window& w) {
    impl_->last_applied_radius = effectiveRadius(w.theme());
}
void WebView::navigateNative(const std::string&) {}
bool WebView::setHtmlNative(const std::string&, const std::string&) { return false; }
bool WebView::evalJSNative(const std::string&, int) { return false; }
void WebView::tickNative() {}

#endif
}  // namespace fluffy
#endif
