// src/tooltip.cpp
// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
// Tooltip — hover overlay
// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
#include <fluffy/tooltip.hpp>

#include <nanovg.h>

#include <fluffy/container.hpp>
#include <fluffy/label.hpp>
#include <fluffy/window.hpp>

#include <unordered_map>

namespace fluffy {

// ––––––––[ Tooltip ] constructs
Tooltip::Tooltip(Rect bounds, std::string text)
    : Widget(bounds), text_(std::move(text)), inline_(Inline{} << text_) {
    fade_anim_.snapTo(0.0f);
    fade_anim_.setTarget(1.0f);
}

// ––––––––[ Tooltip ] inline constructs
Tooltip::Tooltip(Rect bounds, Inline inl)
    : Widget(bounds), inline_(std::move(inl)) {
    std::string plain;
    for (auto& r : inline_.runs()) if (r.type == Inline::Run::Type::Text) plain += r.utf8;
    text_ = std::move(plain);
    fade_anim_.snapTo(0.0f);
    fade_anim_.setTarget(1.0f);
}

// ––––––––[ setText ] sets string
void Tooltip::setText(std::string t) {
    text_ = t;
    inline_ = Inline{} << t;
}

// ––––––––[ setInline ] sets inline
void Tooltip::setInline(Inline inl) {
    inline_ = std::move(inl);
    std::string plain;
    for (auto& r : inline_.runs()) if (r.type == Inline::Run::Type::Text) plain += r.utf8;
    text_ = std::move(plain);
}

// ––––––––[ naturalWidth ] measures text/inline
float Tooltip::naturalWidth(const Theme& theme, const Window& w) const {
    NVGcontext* vg = w.nvgContext();
    const float size = 12.0f;
    if (!vg) return 80.0f;
    if (!inline_.empty()) {
        std::string spec = font_.empty() ? theme.font_family : font_;
        std::string face;
        const_cast<Window&>(w).prepareFont(vg, spec, face);
        nvgFontSize(vg, size);
        nvgFontFace(vg, face.c_str());
        float tw = 0.0f;
        const float icon_gain = theme.metrics.animation.icon_size_gain;
        const float icon_pad = theme.metrics.animation.icon_padding;
        for (auto& r : inline_.runs()) {
            if (r.type == Inline::Run::Type::SvgIcon) {
                float rs = r.size > 0.0f ? r.size : size;
                tw += rs * icon_gain + 2.0f * rs * icon_gain * icon_pad + 4.0f;
            } else {
                float rs = r.size > 0.0f ? r.size : size;
                nvgFontSize(vg, rs);
                tw += nvgTextBounds(vg, 0, 0, r.utf8.c_str(), nullptr, nullptr);
                nvgFontSize(vg, size);
            }
        }
        return tw + 16.0f;
    }
    if (text_.empty()) return 80.0f;
    std::string spec = font_.empty() ? theme.font_family : font_;
    std::string face;
    const_cast<Window&>(w).prepareFont(vg, spec, face);
    nvgFontSize(vg, size);
    nvgFontFace(vg, face.c_str());
    float tw = nvgTextBounds(vg, 0, 0, text_.c_str(), nullptr, nullptr);
    return tw + 16.0f;
}

// ––––––––[ naturalHeight ] fixed
float Tooltip::naturalHeight(const Theme&, const Window&) const { return 24.0f; }

// ––––––––[ draw ] rounded tooltip bubble with fade
void Tooltip::draw(NVGcontext* vg, const Theme& theme, const Window& w) {
    if (!visible_ || (text_.empty() && inline_.empty())) return;
    const float a = fade_anim_.value();
    if (a <= 0.01f) return;
    const uint8_t alpha_bg = static_cast<uint8_t>(220.0f * a);
    const uint8_t alpha_fg = static_cast<uint8_t>(255.0f * a);
    nvgBeginPath(vg);
    nvgRoundedRect(vg, bounds_.x, bounds_.y, bounds_.w, bounds_.h, 4.0f);
    const Color ac = accentColor(theme);
    const uint8_t dr = static_cast<uint8_t>(ac.r * 0.55f);
    const uint8_t dg = static_cast<uint8_t>(ac.g * 0.55f);
    const uint8_t db = static_cast<uint8_t>(ac.b * 0.55f);
    nvgFillColor(vg, nvgRGBA(dr, dg, db, alpha_bg));
    nvgFill(vg);
    const float size = 12.0f;
    std::string spec = font_.empty() ? theme.font_family : font_;
    std::string face;
    w.prepareFont(vg, spec, face);
    nvgFontSize(vg, size);
    nvgFontFace(vg, face.c_str());
    if (!inline_.empty()) {
        // Render inline centered
        float tw = 0.0f;
        const float icon_gain = theme.metrics.animation.icon_size_gain;
        const float icon_pad = theme.metrics.animation.icon_padding;
        // Measure to center
        {
            NVGcontext* vg2 = vg;
            for (auto& r : inline_.runs()) {
                if (r.type == Inline::Run::Type::SvgIcon) {
                    float rs = r.size > 0.0f ? r.size : size;
                    tw += rs * icon_gain + 2.0f * rs * icon_gain * icon_pad + 2.0f;
                } else {
                    float rs = r.size > 0.0f ? r.size : size;
                    nvgFontSize(vg2, rs);
                    tw += nvgTextBounds(vg2, 0, 0, r.utf8.c_str(), nullptr, nullptr);
                    nvgFontSize(vg2, size);
                }
            }
        }
        float cx = bounds_.x + (bounds_.w - tw) * 0.5f;
        float cy = bounds_.y + bounds_.h * 0.5f;
        Color fg{255, 255, 255, alpha_fg};
        renderInline(vg, inline_.runs(), cx, cy,
                     NVG_ALIGN_LEFT, NVG_ALIGN_MIDDLE, size,
                     theme.metrics.animation.icon_size_gain, theme.metrics.animation.icon_padding,
                     face.c_str(), fg, fg);
    } else {
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, nvgRGBA(255, 255, 255, alpha_fg));
        nvgText(vg, bounds_.x + bounds_.w * 0.5f, bounds_.y + bounds_.h * 0.5f, text_.c_str(), nullptr);
    }
    (void)theme;
}

// ––––––––[ tick ] fade and auto-hide after delay
void Tooltip::tick(float dt, const Theme& theme) noexcept {
    timer_ += dt;
    if (!closing_ && timer_ >= auto_hide_) {
        closing_ = true;
        fade_anim_.setTarget(0.0f);
    }
    fade_anim_.tick(dt, theme.metrics.animation.hover_tau);
    if (closing_ && fade_anim_.value() <= 0.01f) {
        if (window_ && window_->hasPopup() && window_->popup() == this) {
            window_->closePopup();
        }
    }
    if (!closing_) {
        fade_anim_.setTarget(1.0f);
        fade_anim_.tick(dt, theme.metrics.animation.hover_tau);
    }
}

// ––––––––[ showFor ] shows popup tooltip near anchor
void Tooltip::showFor(Widget* anchor, const std::string& text, Window& w) {
    if (!anchor || text.empty()) return;
    showFor(anchor, Inline{} << text, w);
}

// ––––––––[ showForInline ] shows inline tooltip
void Tooltip::showFor(Widget* anchor, const Inline& inl, Window& w) {
    if (!anchor || inl.empty()) return;
    std::string plain;
    for (auto& r : inl.runs()) if (r.type == Inline::Run::Type::Text) plain += r.utf8;
    if (w.hasPopup() && w.popup() && w.popup()->isTooltip()) {
        auto* existing = static_cast<Tooltip*>(w.popup());
        if (existing->anchor_ == anchor && existing->inline_ .runs().size() == inl.runs().size()) {
            // Quick check: if same content, keep
            bool same = true;
            auto& a = existing->inline_.runs();
            auto& b = inl.runs();
            if (a.size() != b.size()) same = false;
            else {
                for (size_t i = 0; i < a.size(); ++i) {
                    if (a[i].type != b[i].type || a[i].utf8 != b[i].utf8) { same = false; break; }
                }
            }
            if (same) return;
        }
        w.closePopup();
    }
    Rect b = anchor->bounds();
    auto tip = std::make_unique<Tooltip>(Rect{b.x, b.y - 28, 120, 22}, inl);
    tip->anchor_ = anchor;
    tip->timer_ = 0.0f;
    tip->auto_hide_ = 2.5f;
    tip->closing_ = false;
    tip->fade_anim_.snapTo(0.0f);
    tip->fade_anim_.setTarget(1.0f);
    float tw = tip->naturalWidth(w.theme(), w);
    float th = tip->naturalHeight(w.theme(), w);
    float tx = b.x + (b.w - tw) * 0.5f;
    float ty = b.y - th - 6.0f;
    if (ty < 0) ty = b.y + b.h + 6;
    tip->setBounds(Rect{tx, ty, tw, th});
    w.showPopup(std::move(tip));
}

// ––––––––[ inlineTooltipMap ] stores inline tooltips per widget
static std::unordered_map<Widget*, Inline>& inlineTooltipMap() {
    static std::unordered_map<Widget*, Inline> m;
    return m;
}

// ––––––––[ getInlineTooltip ] retrieves inline tooltip if any
Inline getInlineTooltip(Widget* w) {
    auto& m = inlineTooltipMap();
    auto it = m.find(w);
    if (it != m.end()) return it->second;
    return Inline{};
}

// ––––––––[ setTooltip ] attaches hover handler (simple: stores text in userdata)
void setTooltip(Widget* w, std::string text) {
    if (!w) return;
    w->userdata() = std::move(text);
    // Clear inline version
    inlineTooltipMap().erase(w);
    (void)w;
}

// ––––––––[ setTooltipInline ] attaches inline hover
void setTooltip(Widget* w, Inline inl) {
    if (!w) return;
    std::string plain;
    for (auto& r : inl.runs()) if (r.type == Inline::Run::Type::Text) plain += r.utf8;
    w->userdata() = plain;
    if (inl.empty()) inlineTooltipMap().erase(w);
    else inlineTooltipMap()[w] = std::move(inl);
}

}  // namespace fluffy
