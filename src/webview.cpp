// src/webview.cpp
// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
// WebView — common overlay logic (platform impl in webview_*.cpp/mm)
// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
#include <fluffy/webview.hpp>
#include <fluffy/window.hpp>
#include <fluffy/yaml_fwd.hpp>

#include <cstdlib>

#include <GLFW/glfw3.h>
#if !defined(FLUFFY_EMSCRIPTEN) && !defined(__EMSCRIPTEN__)
#if defined(__APPLE__)
#  define GLFW_EXPOSE_NATIVE_COCOA
#include <objc/runtime.h>
#include <objc/message.h>
#elif defined(__linux__)
#  define GLFW_EXPOSE_NATIVE_X11
#elif defined(_WIN32)
#  define GLFW_EXPOSE_NATIVE_WIN32
#endif
#include <GLFW/glfw3native.h>
#endif

namespace fluffy {

WebView::WebView(Rect bounds, std::string url)
    : Widget(bounds), impl_(std::make_unique<Impl>()), url_(std::move(url)) {}

WebView::~WebView() {
    destroyNative();
}

void WebView::draw(NVGcontext* vg, const Theme& theme, const Window& w) {
    if (!visible_) return;
    if (!impl_->created) {
        if (window_) createNative(w);
    } else {
        syncBounds(w);
        // Keep native rounded clip in sync with theme (track automatically)
        float eff = effectiveRadius(theme);
        if (eff != impl_->last_applied_radius) applyCornerRadius(w);
    }
    if (!impl_->native) {
        const float r = effectiveRadius(theme);
        nvgBeginPath(vg);
        nvgRoundedRect(vg, bounds_.x, bounds_.y, bounds_.w, bounds_.h, r);
        nvgFillColor(vg, nvgRGBA(theme.colors.surface.r, theme.colors.surface.g, theme.colors.surface.b, theme.colors.surface.a));
        nvgFill(vg);
        nvgStrokeColor(vg, nvgRGBA(theme.colors.border.r, theme.colors.border.g, theme.colors.border.b, theme.colors.border.a));
        nvgStrokeWidth(vg, 1.0f);
        nvgStroke(vg);
        nvgFillColor(vg, nvgRGBA(theme.colors.on_surface_mute.r, theme.colors.on_surface_mute.g, theme.colors.on_surface_mute.b, theme.colors.on_surface_mute.a));
        nvgFontSize(vg, 11.0f);
        std::string face;
        w.prepareFont(vg, theme.font_family, face);
        nvgFontFace(vg, face.c_str());
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        std::string disp = url_;
        if (disp.size() > 64) disp = disp.substr(0, 64) + "...";
        if (disp.rfind("data:text/html,", 0) == 0) disp = disp.substr(15, 48) + "...";
        nvgText(vg, bounds_.x + bounds_.w * 0.5f, bounds_.y + bounds_.h * 0.5f, disp.c_str(), nullptr);
        // Hint
        nvgFontSize(vg, 10.0f);
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_TOP);
        nvgText(vg, bounds_.x + bounds_.w * 0.5f, bounds_.y + bounds_.h * 0.5f + 14, "(WebView placeholder — native not yet created)", nullptr);
    }
}

EventResult WebView::handleEvent(const Event& e, const Window& w) {
    // Overlay captures input natively — block Fluffy hover when inside
    float mx{}, my{};
    w.cursorPosition(mx, my);
    const bool inside = containsPoint(mx, my);
    if (inside) {
        // Allow focus on click
        if (kind_of(e) == EventKind::MouseDown) {
            if (window_) window_->requestFocus(this);
        }
        return EventResult::Consumed;
    }
    return EventResult::Ignored;
}

void WebView::tick(float, const Theme& theme) noexcept {
    tickNative();
    if (!window_ || !impl_ || !impl_->created) return;
    float eff = effectiveRadius(theme);
    if (eff != impl_->last_applied_radius) {
        if (window_) applyCornerRadius(*window_);
    }
}

float WebView::effectiveRadius(const Theme& theme) const noexcept {
    if (cornerRadius_ < -0.5f) return theme.metrics.corner_radius;
    return cornerRadius_;
}

void WebView::setCornerRadius(float r) noexcept {
    cornerRadius_ = r;
    if (window_ && impl_ && impl_->created) applyCornerRadius(*window_);
}

float WebView::naturalWidth(const Theme&, const Window&) const {
    // Use explicit bounds width if set, otherwise 320
    return bounds_.w > 1.0f ? bounds_.w : 320.0f;
}
float WebView::naturalHeight(const Theme&, const Window&) const {
    return bounds_.h > 1.0f ? bounds_.h : 320.0f;
}

void WebView::setProperty(std::string_view name, const YamlNode& value) {
    if (name == "url") {
        navigate(value.asString());
        return;
    }
    if (name == "html") {
        setHtml(value.asString(), impl_->pending_base_url);
        return;
    }
    if (name == "base_url" || name == "baseUrl") {
        impl_->pending_base_url = value.asString();
        if (impl_->has_pending_html) {
            setHtml(impl_->pending_html, impl_->pending_base_url);
        }
        return;
    }
    if (name == "radius" || name == "corner_radius" || name == "cornerRadius") {
        setCornerRadius(value.asFloat());
        return;
    }
    Widget::setProperty(name, value);
}

void WebView::navigate(const std::string& url) {
    url_ = url;
    if (impl_) impl_->has_pending_html = false;
    navigateNative(url);
}

void WebView::setHtml(const std::string& html) {
    setHtml(html, "");
}

void WebView::setHtml(const std::string& html, const std::string& baseFileUrl) {
    url_ = "data:text/html," + html;
    if (impl_) {
        impl_->pending_html = html;
        impl_->pending_base_url = baseFileUrl;
        impl_->has_pending_html = true;
    }
    setHtmlNative(html, baseFileUrl);
}

void WebView::evalJS(const std::string& js, std::function<void(const std::string&)> cb) {
    int id = -1;
    if (cb) {
        impl_->pending_evals.push_back(std::move(cb));
        id = static_cast<int>(impl_->pending_evals.size()) - 1;
    }
    if (!evalJSNative(js, id) && id >= 0) {
        handleBridgeMessage("__fluffy_eval:" + std::to_string(id) +
                            ":e:no native webview");
    }
}

// ––––––––[ handleBridgeMessage ] routes bridge payloads to eval callbacks or on_message_
void WebView::handleBridgeMessage(const std::string& payload) {
    const std::string prefix = "__fluffy_eval:";
    if (payload.rfind(prefix, 0) == 0) {
        const std::size_t rest = prefix.size();
        const std::size_t sep = payload.find(':', rest);
        if (sep == std::string::npos || sep + 2 >= payload.size() ||
            payload[sep + 2] != ':') return;
        const int id = std::atoi(payload.substr(rest, sep - rest).c_str());
        const char kind = payload[sep + 1];
        const std::string value = payload.substr(sep + 3);
        if (id >= 0 && static_cast<std::size_t>(id) < impl_->pending_evals.size()) {
            auto cb = std::move(impl_->pending_evals[static_cast<std::size_t>(id)]);
            if (cb) cb(kind == 'e' ? std::string("error: ") + value : value);
        }
        return;
    }
    if (on_message_) on_message_(payload, *this);
}

#ifndef FLUFFY_ENABLE_WEBVIEW
// ––––––––[ createNative ] stub when webview disabled — placeholder only.
void WebView::createNative(const Window&) {}
// ––––––––[ destroyNative ] stub.
void WebView::destroyNative() { if (impl_) { impl_->native = nullptr; impl_->controller = nullptr; impl_->created = false; } }
// ––––––––[ syncBounds ] stub.
void WebView::syncBounds(const Window&) {}
// ––––––––[ applyCornerRadius ] stub.
void WebView::applyCornerRadius(const Window& w) { if (impl_) impl_->last_applied_radius = effectiveRadius(w.theme()); }
// ––––––––[ navigateNative ] stub.
void WebView::navigateNative(const std::string&) {}
// ––––––––[ setHtmlNative ] stub.
bool WebView::setHtmlNative(const std::string&, const std::string&) { return false; }
// ––––––––[ evalJSNative ] stub.
bool WebView::evalJSNative(const std::string&, int) { return false; }
// ––––––––[ tickNative ] stub.
void WebView::tickNative() {}
#endif

}  // namespace fluffy
