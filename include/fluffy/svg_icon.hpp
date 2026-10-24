// include/fluffy/svg_icon.hpp

// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

// SvgIcon — lightweight handle to a Lucide SVG icon's raw XML data.

#pragma once
#include <cstddef>
#include <nanovg.h>
namespace fluffy { namespace Icons {
struct SvgIconRef {
    const char* svg_data;
    std::size_t svg_size;
};
void renderSvgIcon(NVGcontext* vg, const SvgIconRef& icon,
                     float x, float y, float size, NVGcolor color);
}}  

