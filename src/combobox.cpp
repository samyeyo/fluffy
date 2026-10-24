// src/combobox.cpp
// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
// ComboBox — draw, handleEvent, popup list
// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
#include <fluffy/combobox.hpp>

#include <algorithm>
#include <iterator>
#include <string>

#include <nanovg.h>

#include <GLFW/glfw3.h>

#include <fluffy/icons.hpp>
#include <fluffy/menu.hpp>
#include <fluffy/window.hpp>
#include <fluffy/yaml_fwd.hpp>

namespace fluffy {

// ––––––––[ ComboBox ] constructs with bounds
ComboBox::ComboBox(Rect bounds)
    : Widget(bounds) {}

// ––––––––[ setItems ] replaces items
void ComboBox::setItems(std::vector<std::string> items) {
    items_ = items;
    inlineItems_.clear();
    inlineItems_.reserve(items.size());
    for (auto& s : items) inlineItems_.push_back(Inline{} << s);
    if (selected_ >= static_cast<int>(items_.size())) selected_ = -1;
}

// ––––––––[ setItemsInline ] replaces inline items
void ComboBox::setItemsInline(std::vector<Inline> items) {
    inlineItems_ = std::move(items);
    items_.clear();
    items_.reserve(inlineItems_.size());
    for (auto& inl : inlineItems_) {
        std::string plain;
        for (auto& r : inl.runs()) if (r.type == Inline::Run::Type::Text) plain += r.utf8;
        items_.push_back(std::move(plain));
    }
    if (selected_ >= static_cast<int>(items_.size())) selected_ = -1;
}

// ––––––––[ setSelectedIndex ] selects index
void ComboBox::setSelectedIndex(int idx) {
    if (idx < -1 || idx >= static_cast<int>(items_.size())) idx = -1;
    if (idx == selected_) return;
    selected_ = idx;
    if (on_change_) {
        const std::string& v = selectedValue();
        on_change_(selected_, v);
    }
}

// ––––––––[ setPlaceholder ] sets placeholder
void ComboBox::setPlaceholder(std::string p) { placeholder_ = p; placeholderInline_ = Inline{} << p; }

// ––––––––[ setPlaceholderInline ] sets inline placeholder
void ComboBox::setPlaceholderInline(Inline p) {
    placeholderInline_ = p;
    std::string plain;
    for (auto& r : p.runs()) if (r.type == Inline::Run::Type::Text) plain += r.utf8;
    placeholder_ = std::move(plain);
}

// ––––––––[ selectedValue ] returns selected string or empty
const std::string& ComboBox::selectedValue() const noexcept {
    static const std::string kEmpty{};
    if (selected_ >= 0 && selected_ < static_cast<int>(items_.size())) return items_[static_cast<std::size_t>(selected_)];
    return kEmpty;
}

// ––––––––[ selectedInline ] returns Inline or empty
Inline ComboBox::selectedInline() const noexcept {
    if (selected_ >= 0 && selected_ < static_cast<int>(inlineItems_.size())) return inlineItems_[static_cast<std::size_t>(selected_)];
    return Inline{};
}

// ––––––––[ naturalWidth ] measures longest item + padding + chevron
float ComboBox::naturalWidth(const Theme& theme, const Window& w) const {
    NVGcontext* vg = w.nvgContext();
    if (!vg) return 120.0f;
    const float size = textSize_ > 0.0f ? textSize_ : theme.metrics.text_size;
    const std::string spec = font_.empty() ? theme.font_family : font_;
    std::string face;
    const_cast<Window&>(w).prepareFont(vg, spec, face);
    nvgFontSize(vg, size);
    nvgFontFace(vg, face.c_str());
    auto measureInline = [&](const Inline& inl) -> float {
        if (inl.empty()) return 0.0f;
        float tw = 0.0f;
        const float icon_gain = theme.metrics.animation.icon_size_gain;
        const float icon_pad = theme.metrics.animation.icon_padding;
        for (auto& r : inl.runs()) {
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
        return tw;
    };
    float max_w = 0.0f;
    if (!placeholderInline_.empty()) max_w = measureInline(placeholderInline_);
    else max_w = nvgTextBounds(vg, 0, 0, placeholder_.c_str(), nullptr, nullptr);
    for (auto& inl : inlineItems_) max_w = std::max(max_w, measureInline(inl));
    // Fallback to string items if inline empty
    if (inlineItems_.empty()) {
        for (auto& s : items_) {
            float tw = nvgTextBounds(vg, 0, 0, s.c_str(), nullptr, nullptr);
            max_w = std::max(max_w, tw);
        }
    }
    return max_w + theme.metrics.button_padding_x * 2.0f + size * 1.5f + 8.0f;
}

// ––––––––[ naturalHeight ] fixed input height
float ComboBox::naturalHeight(const Theme& theme, const Window&) const {
    const float size = textSize_ > 0.0f ? textSize_ : theme.metrics.text_size;
    return std::max(size + theme.metrics.input_padding_y * 2.0f, theme.metrics.input_min_height);
}

// ––––––––[ showPopup ] builds Menu popup from items
void ComboBox::showPopup(const Window& w) {
    if (items_.empty() && inlineItems_.empty()) return;
    auto menu = std::make_unique<Menu>(Rect{}, Menu::Orientation::Vertical);
    const std::size_t n = std::max(items_.size(), inlineItems_.size());
    for (std::size_t i = 0; i < n; ++i) {
        Menu::Item it;
        if (i < inlineItems_.size() && !inlineItems_[i].empty()) {
            it.inlineLabel = inlineItems_[i];
        } else if (i < items_.size()) {
            it.label = items_[i];
        }
        it.on_click = [this, i]() { setSelectedIndex(static_cast<int>(i)); };
        // Only show check icon for plain string items; inline items already may contain glyphs
        if (static_cast<int>(i) == selected_ && it.inlineLabel.empty()) {
            it.icon = Icons::check;
        }
        menu->addItem(std::move(it));
    }
    auto popup = Menu::makePopup(menu->items(), const_cast<Window&>(w));
    const float nw = popup->naturalWidth(w.theme(), w);
    const float nh = popup->naturalHeight(w.theme(), w);
    float px = bounds_.x;
    float py = bounds_.y + bounds_.h;
    auto [winW, winH] = w.designSize();
    float max_h = nh;
    if (py + nh > static_cast<float>(winH))
        max_h = std::max(0.0f, static_cast<float>(winH) - py - 4.0f);
    if (px + nw > static_cast<float>(winW))
        px = static_cast<float>(winW) - nw - 4.0f;
    if (px < 0) px = 0;
    popup->setBounds(Rect{px, py, nw, std::max(nh, max_h)});
    std::unique_ptr<Widget> widget;
    if (max_h < nh) {
        auto scroll = std::make_unique<Container>(Rect{px, py, nw, max_h});
        scroll->setScrollable(true);
        scroll->setAutoSize(AutoSize::Disabled);
        auto vl = std::make_unique<VerticalLayout>();
        scroll->setLayout(std::move(vl));
        popup->setBounds(Rect{0, 0, nw, nh});
        scroll->addChild(std::move(popup));
        widget = std::move(scroll);
    } else {
        widget = std::move(popup);
    }
    const_cast<Window&>(w).showPopup(std::move(widget));
}

// ––––––––[ draw ] renders ComboBox field
void ComboBox::draw(NVGcontext* vg, const Theme& theme, const Window& w) {
    if (!visible_) return;
    const float t_h = hover_anim_.value();
    const float r = theme.metrics.corner_radius;
    nvgBeginPath(vg);
    nvgRoundedRect(vg, bounds_.x, bounds_.y, bounds_.w, bounds_.h, r);
    Color bg = theme.colors.surface;
    nvgFillColor(vg, nvgRGBA(bg.r, bg.g, bg.b, bg.a));
    nvgFill(vg);
    const Color c_accent = accentColor(theme);
    Color br = focused_ ? c_accent : theme.colors.border;
    nvgStrokeWidth(vg, focused_ ? theme.metrics.input_focus_width : theme.metrics.border_width);
    nvgStrokeColor(vg, nvgRGBA(br.r, br.g, br.b, br.a));
    nvgStroke(vg);
    const float size = textSize_ > 0.0f ? textSize_ : theme.metrics.text_size;
    const std::string spec = font_.empty() ? theme.font_family : font_;
    std::string face;
    w.prepareFont(vg, spec, face);
    nvgFontSize(vg, size);
    nvgFontFace(vg, face.c_str());
    nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
    const float tx = bounds_.x + theme.metrics.input_padding_x + 2.0f;
    const float ty = bounds_.y + bounds_.h * 0.5f;
    const bool has_sel = selected_ >= 0 && selected_ < std::ssize(inlineItems_);
    const bool has_sel_str = selected_ >= 0 && selected_ < std::ssize(items_);
    const float chev_w = size * 1.2f;
    const float max_tx_w = bounds_.w - theme.metrics.input_padding_x * 2.0f - chev_w - 6.0f;
    nvgSave(vg);
    nvgIntersectScissor(vg, bounds_.x, bounds_.y, max_tx_w + theme.metrics.input_padding_x, bounds_.h);
    if (has_sel && !inlineItems_[static_cast<std::size_t>(selected_)].empty()) {
        Color tc = theme.colors.on_surface;
        renderInline(vg, inlineItems_[static_cast<std::size_t>(selected_)].runs(), tx, ty,
                     NVG_ALIGN_LEFT, NVG_ALIGN_MIDDLE, size,
                     theme.metrics.animation.icon_size_gain, theme.metrics.animation.icon_padding,
                     face.c_str(), tc, tc);
    } else if (has_sel_str) {
        const std::string& txt = items_[static_cast<std::size_t>(selected_)];
        Color tc = theme.colors.on_surface;
        nvgFillColor(vg, nvgRGBA(tc.r, tc.g, tc.b, tc.a));
        nvgText(vg, tx, ty, txt.c_str(), nullptr);
    } else {
        // placeholder
        if (!placeholderInline_.empty()) {
            Color tc = theme.colors.on_surface_mute;
            renderInline(vg, placeholderInline_.runs(), tx, ty,
                         NVG_ALIGN_LEFT, NVG_ALIGN_MIDDLE, size,
                         theme.metrics.animation.icon_size_gain, theme.metrics.animation.icon_padding,
                         face.c_str(), tc, tc);
        } else {
            Color tc = theme.colors.on_surface_mute;
            nvgFillColor(vg, nvgRGBA(tc.r, tc.g, tc.b, tc.a));
            nvgText(vg, tx, ty, placeholder_.c_str(), nullptr);
        }
    }
    nvgRestore(vg);
    float chev_s = size * 0.7f;
    float chev_x = bounds_.x + bounds_.w - chev_s - 8.0f;
    float chev_y = bounds_.y + (bounds_.h - chev_s) * 0.5f;
    NVGcolor cc = nvgRGBA(theme.colors.on_surface.r, theme.colors.on_surface.g, theme.colors.on_surface.b, 140);
    Icons::renderIcon(vg, Icons::chevron_down, chev_x, chev_y, chev_s, cc);
    (void)t_h;
}

// ––––––––[ updateHover ] tracks hover
void ComboBox::updateHover(float mx, float my) noexcept {
    hovered_ = containsPoint(mx, my);
    if (hovered_ && window_) window_->setCursorShape(CursorShape::Arrow);
}

// ––––––––[ tick ] animates hover/press
void ComboBox::tick(float dt, const Theme& theme) noexcept {
    hover_anim_.setTarget(hovered_ ? 1.0f : 0.0f);
    press_anim_.setTarget(pressed_ ? 1.0f : 0.0f);
    hover_anim_.tick(dt, theme.metrics.animation.hover_tau);
    press_anim_.tick(dt, theme.metrics.animation.press_tau);
    if (window_) hadPopup_ = window_->hasPopup();
}

// ––––––––[ setFocused ] focus handling
void ComboBox::setFocused(bool f) noexcept { focused_ = f; }

// ––––––––[ handleEvent ] click to popup, keyboard
EventResult ComboBox::handleEvent(const Event& e, const Window& w) {
    if (!enabled_) return EventResult::Ignored;
    float mx{}, my{};
    w.cursorPosition(mx, my);
    const bool inside = containsPoint(mx, my);
    switch (kind_of(e)) {
    case EventKind::MouseDown: {
        const bool curHasPopup = window_ && window_->hasPopup();
        if (inside) {
            // Window already closed popup on PRESS before dispatch if click outside popup.
            // Detect toggle-close flash: popup was open last frame but now closed -> suppress reopen.
            if (hadPopup_ && !curHasPopup) {
                suppressNextOpen_ = true;
                pressed_ = true;
                if (window_) window_->requestFocus(this);
                hadPopup_ = curHasPopup;
                return EventResult::Consumed;
            }
            if (curHasPopup) {
                window_->closePopup();
                suppressNextOpen_ = false;
                hadPopup_ = false;
                if (window_) window_->requestFocus(this);
                return EventResult::Consumed;
            }
            pressed_ = true;
            if (window_) window_->requestFocus(this);
            hadPopup_ = curHasPopup;
            return EventResult::Consumed;
        }
        hadPopup_ = curHasPopup;
        break;
    }
    case EventKind::MouseUp:
        if (pressed_) {
            pressed_ = false;
            if (inside) {
                if (suppressNextOpen_) {
                    suppressNextOpen_ = false;
                    hadPopup_ = window_ && window_->hasPopup();
                    return EventResult::Consumed;
                }
                if (window_ && window_->hasPopup()) {
                    window_->closePopup();
                    hadPopup_ = false;
                    return EventResult::Consumed;
                }
                showPopup(w);
                hadPopup_ = window_ && window_->hasPopup();
                return EventResult::Consumed;
            }
            suppressNextOpen_ = false;
        }
        break;
    case EventKind::KeyDown: {
        if (!focused_) break;
        const auto& ke = std::get<KeyDownEvent>(e);
        if (ke.key == GLFW_KEY_SPACE || ke.key == GLFW_KEY_ENTER) {
            showPopup(w);
            return EventResult::Consumed;
        }
        if (ke.key == GLFW_KEY_UP) {
            if (selected_ > 0) setSelectedIndex(selected_ - 1);
            return EventResult::Consumed;
        }
        if (ke.key == GLFW_KEY_DOWN) {
            if (selected_ + 1 < std::ssize(items_)) setSelectedIndex(selected_ + 1);
            return EventResult::Consumed;
        }
        break;
    }
    default: break;
    }
    return EventResult::Ignored;
}

// ––––––––[ setProperty ] yaml support
void ComboBox::setProperty(std::string_view name, const YamlNode& value) {
    if (name == "placeholder") { setPlaceholder(value.asString()); return; }
    if (name == "selected" || name == "value") {
        if (value.IsScalar()) {
            const std::string s = value.asString();
            // Try integer index first
            bool is_num = !s.empty() && (s[0] == '-' || (s[0] >= '0' && s[0] <= '9'));
            if (is_num) {
                int idx = value.asInt();
                // If asInt returns 0 for non-numeric, check string
                if (s == "0" || idx != 0) { setSelectedIndex(idx); return; }
            }
            for (size_t i = 0; i < items_.size(); ++i) if (items_[i] == s) { setSelectedIndex(static_cast<int>(i)); break; }
        }
        return;
    }
    if (name == "items" || name == "options") {
        std::vector<std::string> out;
        if (value.IsSequence()) {
            for (const auto& ch : value) out.push_back(ch.asString());
        }
        setItems(std::move(out));
        return;
    }
    this->Widget::setProperty(name, value);
}

}  // namespace fluffy
