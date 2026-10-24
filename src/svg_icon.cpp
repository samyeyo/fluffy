// src/svg_icon.cpp

// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

// SvgIcon — NanoSVG parsing and NanoVG GPU-accelerated rendering.

#include <fluffy/svg_icon.hpp>
#define NANOSVG_IMPLEMENTATION
#include <nanosvg.h>
#include <nanovg.h>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
namespace fluffy { namespace Icons {
namespace {
// Simple single-entry cache: most frames render one icon at a time.
struct SvgCache {
    const char*  key = nullptr;
    NSVGimage*   image = nullptr;
    std::vector<char> mutable_svg;
};
SvgCache s_cache;
}  
void renderSvgIcon(NVGcontext* vg, const SvgIconRef& icon,
                     float x, float y, float size, NVGcolor color) {
    if (!icon.svg_data || icon.svg_size == 0) return;
    NSVGimage* img = nullptr;
    if (s_cache.key == icon.svg_data) {
        img = s_cache.image;
    } else {
        if (s_cache.image) { nsvgDelete(s_cache.image); s_cache.image = nullptr; }
        s_cache.key = icon.svg_data;
        s_cache.mutable_svg.assign(icon.svg_data,
                                    icon.svg_data + icon.svg_size + 1);
        s_cache.mutable_svg.back() = '\0';
        s_cache.image = nsvgParse(s_cache.mutable_svg.data(), "px", 96.0f);
        img = s_cache.image;
    }
    if (!img) return;
    const float scale = (img->width > 0.0f && img->height > 0.0f)
        ? size / img->width
        : size / 24.0f;
    for (NSVGshape* shape = img->shapes; shape; shape = shape->next) {
        if (!(shape->flags & NSVG_FLAGS_VISIBLE)) continue;
        nvgBeginPath(vg);
        for (NSVGpath* path = shape->paths; path; path = path->next) {
            nvgMoveTo(vg,
                      x + path->pts[0] * scale,
                      y + path->pts[1] * scale);
            for (int i = 1; i < path->npts; i += 3) {
                float* p = &path->pts[i * 2];
                nvgBezierTo(vg,
                    x + p[0] * scale, y + p[1] * scale,
                    x + p[2] * scale, y + p[3] * scale,
                    x + p[4] * scale, y + p[5] * scale);
            }
            if (path->closed) {
                nvgClosePath(vg);
            }
        }
        if (shape->fill.type == NSVG_PAINT_COLOR) {
            NVGcolor fill_color = color;
            fill_color.a *= shape->opacity;
            nvgFillColor(vg, fill_color);
            nvgFill(vg);
        }
        if (shape->stroke.type == NSVG_PAINT_COLOR &&
            shape->strokeWidth > 0.0f) {
            NVGcolor stroke_color = color;
            stroke_color.a *= shape->opacity;
            nvgStrokeWidth(vg, shape->strokeWidth * scale);
            nvgStrokeColor(vg, stroke_color);
            nvgStroke(vg);
        }
    }
}
}}  // namespace fluffy::Icons  

