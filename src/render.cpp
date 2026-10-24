// src/render.cpp

// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

// renderInline / drawFocusRing — NanoVG inline text+icon rendering.

#include <fluffy/inline.hpp>
#include <fluffy/svg_icon.hpp>
#include <nanovg.h>
#include <string>
#include <vector>
namespace fluffy {

void renderInline(
    NVGcontext* vg,
    const std::vector<Inline::Run>& runs,
    float anchor_x, float anchor_y,
    int   hflags, int   vflags,
    float base_size,
    float icon_size_gain,
    float icon_padding,
    const char* default_face,
    Color default_text_color,
    Color default_icon_color) {
    if (runs.empty()) return;
    // ––––––––[ bold face ] check if a real bold variant exists for default_face.
    const std::string bold_face_str = std::string(default_face) + "-bold";
    const int bold_font_id = nvgFindFont(vg, bold_face_str.c_str());
    const bool has_bold = bold_font_id != -1;
    std::vector<float> widths;
    widths.reserve(runs.size());
    float total = 0.0f;
    for (const auto& r : runs) {
        const float run_size    = (r.size > 0.0f) ? r.size : base_size;
        const bool  is_svg      = (r.type == Inline::Run::Type::SvgIcon);
        const float scaled_size = is_svg ? (run_size * icon_size_gain)
                                         : run_size;
        float w = 0.0f;
        if (is_svg) {
            w = scaled_size + 2.0f * scaled_size * icon_padding;
        } else {
            nvgFontSize(vg, scaled_size);
            if (r.bold && has_bold) {
                nvgFontFace(vg, bold_face_str.c_str());
            } else {
                nvgFontFace(vg, default_face);
            }
            w = nvgTextBounds(vg, 0.0f, 0.0f, r.utf8.c_str(), nullptr, nullptr);
        }
        widths.push_back(w);
        total += w;
    }
    float x = anchor_x;
    if      (hflags & NVG_ALIGN_CENTER) x -= total * 0.5f;
    else if (hflags & NVG_ALIGN_RIGHT)  x -= total;
    for (std::size_t i = 0; i < runs.size(); ++i) {
        const auto& r = runs[i];
        const bool  is_svg = (r.type == Inline::Run::Type::SvgIcon);
        const float run_size = (r.size > 0.0f) ? r.size : base_size;
        const float scaled_size = is_svg ? (run_size * icon_size_gain)
                                         : run_size;
        if (!is_svg) {
            nvgFontSize(vg, scaled_size);
            if (r.bold && has_bold) {
                nvgFontFace(vg, bold_face_str.c_str());
            } else {
                nvgFontFace(vg, default_face);
            }
        }
        const Color c = r.color.value_or(is_svg ? default_icon_color
                                                : default_text_color);
        const NVGcolor nvg_c = nvgRGBA(c.r, c.g, c.b, c.a);
        if (is_svg) {
            const float x_pad = scaled_size * icon_padding;
            Icons::renderSvgIcon(vg, r.svg_icon,
                            x + x_pad, anchor_y - scaled_size * 0.5f, scaled_size, nvg_c);
        } else {
            nvgFillColor(vg, nvg_c);
            nvgTextAlign(vg, NVG_ALIGN_LEFT | vflags);
            if (r.bold) {
                if (has_bold) {
                    nvgText(vg, x, anchor_y, r.utf8.c_str(), nullptr);
                } else {
                    // Fallback faux bold when no bold face is available.
                    nvgText(vg, x - 0.5f, anchor_y, r.utf8.c_str(), nullptr);
                    nvgText(vg, x + 0.5f, anchor_y, r.utf8.c_str(), nullptr);
                }
            } else if (r.italic) {
                // Faux italic: combined skew + translate in one matrix
                nvgSave(vg);
                nvgTransform(vg, 1.0f, 0.0f, 0.2f, 1.0f, x, anchor_y);
                nvgText(vg, 0.0f, 0.0f, r.utf8.c_str(), nullptr);
                nvgRestore(vg);
            } else {
                nvgText(vg, x, anchor_y, r.utf8.c_str(), nullptr);
            }
        }
        x += widths[i];
    }
}

void drawFocusRing(NVGcontext* vg, const Rect& bounds, const Theme& theme) {
    const float r = theme.metrics.corner_radius;
    nvgBeginPath(vg);
    nvgRoundedRect(vg, bounds.x - 2.0f, bounds.y - 2.0f,
                    bounds.w + 4.0f, bounds.h + 4.0f,
                    r + 1.0f);
    nvgStrokeWidth(vg, 2.0f);
    const auto& c = theme.colors.accent;
    nvgStrokeColor(vg, nvgRGBA(c.r, c.g, c.b, 180));
    nvgStroke(vg);
}

}  // namespace fluffy
