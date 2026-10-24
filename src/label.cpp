// src/label.cpp

// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

// Label — renders a single line of text via NanoVG (std::string or Inline).

#include <fluffy/label.hpp>
#include <fluffy/inline.hpp>
#include <fluffy/window.hpp>
#include <fluffy/yaml_fwd.hpp>
#include <nanovg.h>
namespace fluffy {
Label::Label(Rect bounds, std::string text, Align horizontal, Align vertical)
    : Widget(bounds),
      text_(std::move(text)),
      inline_{},
      is_inline_(false),
      h_(horizontal),
      v_(vertical)
{}
Label::Label(Rect bounds, Inline content, Align horizontal, Align vertical)
    : Widget(bounds),
      inline_(std::move(content.runs())),
      is_inline_(true),
      h_(horizontal),
      v_(vertical)
{}
void Label::setText(std::string t) {
    text_      = std::move(t);
    inline_.clear();
    is_inline_ = false;
}
void Label::setInline(Inline c) {
    text_.clear();
    inline_      = std::move(c.runs());
    is_inline_   = true;
}
void Label::setProperty(std::string_view name, const YamlNode& value) {
    if (name == "text")  { setText(value.asString()); return; }
    if (name == "color") {
        const std::string raw = value.asString();
        Color::fromString(raw);
        return;
    }
    if (name == "align" || name == "horizontal_align" ||
        name == "vertical_align") {
        const std::string raw = value.asString();
        const Align a =
            (raw == "Left"   || raw == "left"   || raw == "Top"    || raw == "top")    ? Align::Left
          : (raw == "Right"  || raw == "right"  || raw == "Bottom" || raw == "bottom") ? Align::Right
          : Align::Center;
        if (name == "horizontal_align") setHorizontal(a);
        else if (name == "vertical_align") setVertical(a);
        else                            { setHorizontal(a); setVertical(a); }
        return;
    }
    Widget::setProperty(name, value);
}
float Label::naturalWidth(const Theme& theme, const Window& w) const {
    if (text_.empty() && inline_.empty()) return 0.0f;
    NVGcontext* vg = w.nvgContext();
    if (!vg) return 0.0f;
    const float size = textSize_ > 0.0f ? textSize_ : theme.metrics.text_size;
    const std::string spec = font_.empty() ? theme.font_family : font_;
    std::string face;
    const_cast<Window&>(w).prepareFont(vg, spec, face);
    nvgFontSize(vg, size);
    nvgFontFace(vg, face.c_str());
    const float pad_x = theme.metrics.button_padding_x;
    float content_w;
    if (is_inline_) {
        const float iconGain = theme.metrics.animation.icon_size_gain;
        const float iconPad  = theme.metrics.animation.icon_padding;
        float total = 0.0f;
        for (const auto& r : inline_) {
            if (r.type == Inline::Run::Type::SvgIcon) {
                total += r.size > 0.0f ? r.size * iconGain * (1.0f + 2.0f * iconPad)
                        : size * iconGain * (1.0f + 2.0f * iconPad);
            } else {
                total += nvgTextBounds(vg, 0, 0, r.utf8.c_str(), nullptr, nullptr);
            }
        }
        content_w = total;
    } else {
        content_w = nvgTextBounds(vg, 0.0f, 0.0f, text_.c_str(), nullptr, nullptr);
    }
    return content_w + 2.0f * pad_x;
}
float Label::naturalHeight(const Theme& theme, const Window& w) const {
    const float size = textSize_ > 0.0f ? textSize_ : theme.metrics.text_size;
    if (!word_wrap_ || text_.empty() || bounds_.w <= 0.0f) {
        return size + 2.0f * theme.metrics.button_padding_y;
    }
    NVGcontext* vg = w.nvgContext();
    if (!vg) return size + 2.0f * theme.metrics.button_padding_y;
    const std::string spec = font_.empty() ? theme.font_family : font_;
    std::string face;
    const_cast<Window&>(w).prepareFont(vg, spec, face);
    nvgFontSize(vg, size);
    nvgFontFace(vg, face.c_str());
    const float pad_x = theme.metrics.button_padding_x;
    const float break_w = bounds_.w - 2.0f * pad_x;
    if (break_w <= 0.0f) return size + 2.0f * theme.metrics.button_padding_y;
    NVGtextRow rows[256];
    int count = nvgTextBreakLines(vg, text_.c_str(), nullptr, break_w, rows, 256);
    if (count <= 0) count = 1;
    const float line_h = size * 1.4f;
    return static_cast<float>(count) * line_h + 2.0f * theme.metrics.button_padding_y;
}
void Label::draw(NVGcontext* vg, const Theme& theme, const Window& w) {
    if (!visible_) return;
    if (!is_inline_ && text_.empty()) return;
    if ( is_inline_ && inline_.empty()) return;
    const std::string spec = font_.empty() ? theme.font_family : font_;
    std::string face;
    w.prepareFont(vg, spec, face);
    const float size = textSize_ > 0.0f ? textSize_
                                         : theme.metrics.text_size;
    int hflags = NVG_ALIGN_LEFT;
    float anchor_x = bounds_.left();
    switch (h_) {
    case Align::Left:   hflags = NVG_ALIGN_LEFT;   anchor_x = bounds_.left();        break;
    case Align::Center: hflags = NVG_ALIGN_CENTER; anchor_x = bounds_.center_x();    break;
    case Align::Right:  hflags = NVG_ALIGN_RIGHT;  anchor_x = bounds_.right();       break;
    }
    int vflags = NVG_ALIGN_BASELINE;
    float anchor_y = bounds_.center_y();
    switch (v_) {
    case Align::Left:   vflags = NVG_ALIGN_TOP;     anchor_y = bounds_.top();    break;
    case Align::Center: vflags = NVG_ALIGN_MIDDLE;  anchor_y = bounds_.center_y(); break;
    case Align::Right:  vflags = NVG_ALIGN_BOTTOM;  anchor_y = bounds_.bottom(); break;
    }
    const float icon_gain  = theme.metrics.animation.icon_size_gain;
    const float icon_pad_t = theme.metrics.animation.icon_padding;
    const Color default_text = theme.colors.on_surface;
    const Color default_icon = accentColor(theme);
    const float pad_x = theme.metrics.button_padding_x;
    const float break_w = bounds_.w - 2.0f * pad_x;
    const bool wrap = word_wrap_ && !is_inline_ && break_w > 0.0f;
    if (wrap) {
        nvgFontSize(vg, size);
        nvgFontFace(vg, face.c_str());
        NVGtextRow rows[256];
        int count = nvgTextBreakLines(vg, text_.c_str(), nullptr, break_w, rows, 256);
        if (count <= 0) {
            count = 1;
            rows[0].start = text_.c_str();
            rows[0].end   = text_.c_str() + text_.size();
            rows[0].next  = rows[0].end;
            rows[0].width = nvgTextBounds(vg, 0.0f, 0.0f, text_.c_str(), nullptr, nullptr);
        }
        const float line_h = size * 1.4f;
        float y = anchor_y;
        if (v_ == Align::Center) y = bounds_.center_y() - (static_cast<float>(count) * line_h) * 0.5f;
        else if (v_ == Align::Right) y = bounds_.bottom() - static_cast<float>(count) * line_h;
        const NVGcolor nvg_c = nvgRGBA(default_text.r, default_text.g, default_text.b, default_text.a);
        for (int i = 0; i < count; ++i) {
            const auto& row = rows[i];
            float x = anchor_x;
            if (h_ == Align::Center) x -= row.width * 0.5f;
            else if (h_ == Align::Right) x -= row.width;
            nvgFillColor(vg, nvg_c);
            nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_BASELINE);
            nvgText(vg, x, y + size, row.start, row.end);
            y += line_h;
        }
    } else if (is_inline_) {
        renderInline(vg, inline_,
                      anchor_x, anchor_y,
                      hflags, vflags,
                      size,
                      icon_gain,
                      icon_pad_t,
                      face.c_str(),
                      default_text,
                      default_icon);
    } else {
        Inline::Run text_run;
        text_run.type = Inline::Run::Type::Text;
        text_run.utf8 = text_;
        const std::vector<Inline::Run> runs = { std::move(text_run) };
        renderInline(vg, runs,
                      anchor_x, anchor_y,
                      hflags, vflags,
                      size,
                      icon_gain,
                      icon_pad_t,
                      face.c_str(),
                      default_text,
                      default_icon);
    }
}
}  

