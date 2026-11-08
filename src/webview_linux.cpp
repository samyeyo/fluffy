// src/webview_linux.cpp
// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
// WebView — Linux overlay via WebKitGTK (webkit2gtk-4.1)
// Semantic bridge via script-message-received / decide-policy.
// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
#if !defined(__APPLE__) && !defined(_WIN32) && defined(FLUFFY_ENABLE_WEBVIEW)
#include <fluffy/webview.hpp>
#include <fluffy/window.hpp>

#include <gtk/gtk.h>
#include <gtk/gtkx.h>
#include <gdk/gdkx.h>
#include <webkit2/webkit2.h>
#include <JavaScriptCore/JavaScript.h>

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <unordered_map>

namespace fluffy {

// ––––––––[ WebViewBridge ] WebKit script-message glue, friend of WebView.
struct WebViewBridge {
    static void onScriptMessage(WebKitUserContentManager*, WebKitJavascriptResult*, gpointer);
};

namespace {

struct LinuxState {
    WebView* self = nullptr;
    GtkWidget* plug = nullptr;
    GtkWidget* webview = nullptr;
    GtkCssProvider* css = nullptr;
    gulong message_handler = 0;
};

std::unordered_map<WebView*, LinuxState*>& states() {
    static std::unordered_map<WebView*, LinuxState*> map;
    return map;
}

bool initGtk() {
    static const bool ok = [] {
        // The overlay is embedded in a GLFW X11 window, so GTK must use the
        // X11 backend even when the session is Wayland (XWayland).
        gdk_set_allowed_backends("x11");
        int argc = 1;
        char name[] = "fluffy";
        char* argv[] = {name, nullptr};
        char** args = argv;
        return gtk_init_check(&argc, &args) != FALSE;
    }();
    return ok;
}

void pumpGtk() {
    int n = 0;
    while (g_main_context_pending(nullptr) && n++ < 64) {
        g_main_context_iteration(nullptr, FALSE);
    }
}

void sizeWidget(LinuxState& st, float x, float y, float w, float h) {
    if (!st.plug) return;
    int scale = gtk_widget_get_scale_factor(st.plug);
    if (scale < 1) scale = 1;
    int px = static_cast<int>(std::lround(x * scale));
    int py = static_cast<int>(std::lround(y * scale));
    int pw = static_cast<int>(std::lround(w * scale));
    int ph = static_cast<int>(std::lround(h * scale));
    GdkWindow* window = gtk_widget_get_window(st.plug);
    if (window) gdk_window_move_resize(window, px, py, pw, ph);
    gtk_widget_set_size_request(st.webview, static_cast<int>(std::lround(w)), static_cast<int>(std::lround(h)));
    gtk_widget_queue_resize(st.webview);
}

}  // namespace

void WebViewBridge::onScriptMessage(WebKitUserContentManager*, WebKitJavascriptResult* result, gpointer user_data) {
    auto* st = static_cast<LinuxState*>(user_data);
    if (!st || !st->self || !result) return;
    JSCValue* value = webkit_javascript_result_get_js_value(result);
    if (!value) return;
    char* text = jsc_value_to_string(value);
    if (text) {
        st->self->handleBridgeMessage(text);
        g_free(text);
    }
}

// ––––––––[ createNative ] creates a GtkPlug/WebKitWebView inside the GLFW X11 window.
void WebView::createNative(const Window& w) {
    if (impl_->created) return;
    void* raw = w.nativeHandle();
    if (!raw) return;
    if (!initGtk()) return;

    GdkDisplay* display = gdk_display_get_default();
    if (!display || !GDK_IS_X11_DISPLAY(display)) return;

    auto* st = new LinuxState();
    st->self = this;
    st->plug = gtk_plug_new(0);
    if (!st->plug) {
        delete st;
        return;
    }
    gtk_window_set_decorated(GTK_WINDOW(st->plug), FALSE);
    gtk_window_set_skip_taskbar_hint(GTK_WINDOW(st->plug), TRUE);
    gtk_window_set_skip_pager_hint(GTK_WINDOW(st->plug), TRUE);

    st->webview = webkit_web_view_new();
    if (!st->webview) {
        gtk_widget_destroy(st->plug);
        delete st;
        return;
    }
    gtk_widget_set_name(st->webview, "fluffy-webview");
    gtk_widget_set_can_focus(st->webview, TRUE);
    gtk_container_add(GTK_CONTAINER(st->plug), st->webview);

    WebKitUserContentManager* manager =
        webkit_web_view_get_user_content_manager(WEBKIT_WEB_VIEW(st->webview));
    webkit_user_content_manager_register_script_message_handler(manager, "fluffy");
    st->message_handler = g_signal_connect(
        manager, "script-message-received::fluffy",
        G_CALLBACK(WebViewBridge::onScriptMessage), st);

    // Create the plug window but keep it unmapped while reparenting. Mapping a
    // toplevel first lets the window manager claim it, after which the manual
    // reparent either fails or leaves the webview hidden.
    gtk_widget_realize(st->plug);

    GdkWindow* plug_window = gtk_widget_get_window(st->plug);
    GdkWindow* parent_window = gdk_x11_window_foreign_new_for_display(
        display, static_cast<::Window>(reinterpret_cast<uintptr_t>(raw)));
    if (!plug_window || !parent_window) {
        if (parent_window) g_object_unref(parent_window);
        gtk_widget_destroy(st->plug);
        delete st;
        return;
    }
    gdk_window_reparent(plug_window, parent_window, 0, 0);
    g_object_unref(parent_window);
    gtk_widget_show_all(st->plug);
    gdk_window_show(plug_window);
    gdk_window_raise(plug_window);

    states()[this] = st;
    impl_->native = st->webview;
    impl_->controller = st->plug;
    impl_->created = true;
    impl_->last_bounds = bounds_;
    impl_->last_applied_radius = -999.0f;

    sizeWidget(*st, bounds_.x, bounds_.y, bounds_.w, bounds_.h);
    applyCornerRadius(w);

    if (impl_->has_pending_html) {
        webkit_web_view_load_html(WEBKIT_WEB_VIEW(st->webview),
                                  impl_->pending_html.c_str(),
                                  impl_->pending_base_url.empty() ? nullptr
                                                                  : impl_->pending_base_url.c_str());
    } else if (!url_.empty()) {
        webkit_web_view_load_uri(WEBKIT_WEB_VIEW(st->webview), url_.c_str());
    }
    pumpGtk();
}

// ––––––––[ destroyNative ] removes the WebKitWebView and its GtkPlug.
void WebView::destroyNative() {
    auto it = states().find(this);
    if (it != states().end()) {
        LinuxState* st = it->second;
        states().erase(it);
        st->self = nullptr;

        if (st->webview) {
            if (st->css) {
                GtkStyleContext* ctx = gtk_widget_get_style_context(st->webview);
                gtk_style_context_remove_provider(ctx, GTK_STYLE_PROVIDER(st->css));
                g_object_unref(st->css);
                st->css = nullptr;
            }
            WebKitUserContentManager* manager =
                webkit_web_view_get_user_content_manager(WEBKIT_WEB_VIEW(st->webview));
            if (manager) {
                if (st->message_handler) {
                    g_signal_handler_disconnect(manager, st->message_handler);
                    st->message_handler = 0;
                }
                webkit_user_content_manager_unregister_script_message_handler(manager, "fluffy");
            }
        }
        if (st->plug) gtk_widget_destroy(st->plug);
        delete st;
    }
    impl_->native = nullptr;
    impl_->controller = nullptr;
    impl_->created = false;
    impl_->last_applied_radius = -999.0f;
}

// ––––––––[ syncBounds ] repositions the GtkPlug inside the X11 parent.
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
    sizeWidget(*it->second, bounds_.x, bounds_.y, bounds_.w, bounds_.h);
    impl_->last_bounds = bounds_;
    if (changed) applyCornerRadius(w);
    pumpGtk();
}

// ––––––––[ applyCornerRadius ] applies CSS border-radius to the WebKit widget.
void WebView::applyCornerRadius(const Window& w) {
    auto it = states().find(this);
    if (it == states().end()) return;
    LinuxState& st = *it->second;
    if (!st.webview) return;

    float r = effectiveRadius(w.theme());
    impl_->last_applied_radius = r;

    GtkStyleContext* ctx = gtk_widget_get_style_context(st.webview);
    if (st.css) {
        gtk_style_context_remove_provider(ctx, GTK_STYLE_PROVIDER(st.css));
        g_object_unref(st.css);
        st.css = nullptr;
    }
    if (r <= 0.5f) return;

    st.css = gtk_css_provider_new();
    char css[160];
    std::snprintf(css, sizeof(css),
                  "#fluffy-webview { border-radius: %dpx; }",
                  static_cast<int>(std::lround(r)));
    gtk_css_provider_load_from_data(st.css, css, -1, nullptr);
    gtk_style_context_add_provider(ctx, GTK_STYLE_PROVIDER(st.css),
                                   GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
}

// ––––––––[ navigateNative ] loads a URL in WebKitGTK.
void WebView::navigateNative(const std::string& url) {
    if (!impl_ || !impl_->native) return;
    webkit_web_view_load_uri(WEBKIT_WEB_VIEW(impl_->native), url.c_str());
}

// ––––––––[ setHtmlNative ] loads markup with an optional base URL.
bool WebView::setHtmlNative(const std::string& html, const std::string& baseFileUrl) {
    if (!impl_ || !impl_->native) return false;
    webkit_web_view_load_html(WEBKIT_WEB_VIEW(impl_->native), html.c_str(),
                              baseFileUrl.empty() ? nullptr : baseFileUrl.c_str());
    return true;
}

// ––––––––[ evalJSNative ] evaluates JS and posts the result through the bridge.
bool WebView::evalJSNative(const std::string& js, int id) {
    if (!impl_ || !impl_->native) return false;
    const std::string tag = "__fluffy_eval:" + std::to_string(id);
    const std::string wrapped =
        "(function(){try{var r=String((" + js + "));"
        "window.webkit.messageHandlers.fluffy.postMessage(\"" + tag + ":r:\"+r);}"
        "catch(e){window.webkit.messageHandlers.fluffy.postMessage(\"" + tag + ":e:\"+String(e));}})()";
#if WEBKIT_CHECK_VERSION(2, 40, 0)
    webkit_web_view_evaluate_javascript(WEBKIT_WEB_VIEW(impl_->native),
                                        wrapped.c_str(), -1, nullptr, nullptr,
                                        nullptr, nullptr, nullptr);
#else
    webkit_web_view_run_javascript(WEBKIT_WEB_VIEW(impl_->native), wrapped.c_str(),
                                   nullptr, nullptr, nullptr);
#endif
    return true;
}

// ––––––––[ tickNative ] drains pending GTK/WebKit events.
void WebView::tickNative() {
    pumpGtk();
}

}  // namespace fluffy
#endif
