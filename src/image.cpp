// src/image.cpp

// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

// Image — raster image widget; loads via platform decoders, renders via NanoVG.

#include <fluffy/image.hpp>

#include <algorithm>

#include <nanovg.h>

#include <fluffy/window.hpp>
#include <fluffy/yaml_fwd.hpp>

namespace fluffy {

// Platform-specific decoder declaration (implemented in image_mac.cpp / image_win.cpp / image_linux.cpp).
namespace detail {
bool platformLoadImage(const std::string& path,
                       unsigned char*& rgba_out,
                       int& w_out, int& h_out);
}

#if defined(FLUFFY_EMSCRIPTEN) || defined(__EMSCRIPTEN__)
namespace detail {
bool platformLoadImage(const std::string&, unsigned char*&, int&, int&) { return false; }
}
#endif

// ––––––––[ Image ] loads from a file path.
Image::Image(Rect bounds, std::string_view filepath)
    : Widget(bounds) {
    setImage(std::string(filepath));
}

// ––––––––[ Image ] loads from raw RGBA pixel data.
Image::Image(Rect bounds, int width, int height, const unsigned char* rgba)
    : Widget(bounds) {
    setImage(width, height, rgba);
}

// ––––––––[ ~Image ] frees the NanoVG image handle.
Image::~Image() {
    if (image_id_ >= 0 && window_) {
        NVGcontext* vg = window_->nvgContext();
        if (vg) nvgDeleteImage(vg, image_id_);
    }
}

// ––––––––[ setImage ] loads and decodes an image file.
void Image::setImage(const std::string& filepath) {
    clearImage();

    unsigned char* rgba = nullptr;
    int w = 0, h = 0;
    if (!detail::platformLoadImage(filepath, rgba, w, h) || !rgba) return;

    img_w_ = w;
    img_h_ = h;
    pending_rgba_.assign(rgba, rgba + static_cast<size_t>(w) * h * 4);
    std::free(rgba);

    uploadToGPU();
}

// ––––––––[ setImage ] creates an image from raw RGBA data.
void Image::setImage(int w, int h, const unsigned char* rgba) {
    clearImage();
    if (!rgba || w <= 0 || h <= 0) return;

    img_w_ = w;
    img_h_ = h;
    pending_rgba_.assign(rgba, rgba + static_cast<size_t>(w) * h * 4);

    uploadToGPU();
}

// ––––––––[ uploadToGPU ] sends CPU-side RGBA data to the GPU via NanoVG.
void Image::uploadToGPU() {
    if (pending_rgba_.empty() || image_id_ >= 0) return;
    NVGcontext* vg = window_ ? window_->nvgContext() : nullptr;
    if (!vg) return;

    image_id_ = nvgCreateImageRGBA(vg, img_w_, img_h_,
        NVG_IMAGE_PREMULTIPLIED | NVG_IMAGE_FLIPY, pending_rgba_.data());
}

// ––––––––[ clearImage ] frees the NanoVG image handle and resets state.
void Image::clearImage() {
    if (image_id_ >= 0 && window_) {
        NVGcontext* vg = window_->nvgContext();
        if (vg) nvgDeleteImage(vg, image_id_);
    }
    image_id_ = -1;
    img_w_    = 0;
    img_h_    = 0;
    pending_rgba_.clear();
}

// ––––––––[ naturalWidth ] returns the widget's assigned width.
float Image::naturalWidth(const Theme&, const Window&) const {
    return bounds_.w > 0.0f ? bounds_.w : static_cast<float>(img_w_);
}

// ––––––––[ naturalHeight ] returns the widget's assigned height.
float Image::naturalHeight(const Theme&, const Window&) const {
    return bounds_.h > 0.0f ? bounds_.h : static_cast<float>(img_h_);
}

// ––––––––[ draw ] renders the image with the current scale mode.
void Image::draw(NVGcontext* vg, const Theme& theme, const Window& w) {
    (void)theme;
    (void)w;
    if (!visible_) return;

    // Deferred GPU upload — window_ (and thus NVG context) is now available.
    if (image_id_ < 0) uploadToGPU();
    if (image_id_ < 0) return;

    const float bx = bounds_.x;
    const float by = bounds_.y;
    const float bw = bounds_.w;
    const float bh = bounds_.h;

    float ox = bx, oy = by, ex = bw, ey = bh;

    if (scale_mode_ == ScaleMode::Fit || scale_mode_ == ScaleMode::Fill) {
        const float img_aspect = static_cast<float>(img_w_) /
                                 static_cast<float>(img_h_);
        const float box_aspect = bw / bh;

        float scale;
        if ((scale_mode_ == ScaleMode::Fit) == (img_aspect < box_aspect)) {
            scale = bw / static_cast<float>(img_w_);
        } else {
            scale = bh / static_cast<float>(img_h_);
        }

        ex = static_cast<float>(img_w_) * scale;
        ey = static_cast<float>(img_h_) * scale;
        ox = bx + (bw - ex) * 0.5f;
        oy = by + (bh - ey) * 0.5f;
    }

    NVGpaint imgPaint = nvgImagePattern(vg, ox, oy, ex, ey,
                                         0.0f, image_id_, 1.0f);

    const float cr = theme.metrics.corner_radius;

    if (border_) {
        // Shadow (behind everything)
        const Shadow& sh = theme.metrics.shadow;
        if (sh.blur > 0.0f && sh.color.a > 0) {
            const float sx = bx + sh.offset_x;
            const float sy = by + sh.offset_y;
            const float ext = sh.blur;
            NVGcolor sh_inner = nvgRGBA(sh.color.r, sh.color.g,
                                         sh.color.b, sh.color.a);
            NVGcolor sh_outer = nvgRGBA(sh.color.r, sh.color.g,
                                         sh.color.b, 0);
            NVGpaint sh_paint = nvgBoxGradient(vg,
                sx, sy, bw, bh, cr, ext, sh_inner, sh_outer);
            nvgBeginPath(vg);
            nvgRect(vg, sx - ext, sy - ext, bw + 2.0f * ext, bh + 2.0f * ext);
            nvgFillPaint(vg, sh_paint);
            nvgFill(vg);
        }
    }

    nvgBeginPath(vg);
    if (border_) {
        nvgRoundedRect(vg, bx, by, bw, bh, cr);
    } else {
        nvgRect(vg, bx, by, bw, bh);
    }
    nvgFillPaint(vg, imgPaint);
    nvgFill(vg);

    if (border_) {
        nvgBeginPath(vg);
        nvgRoundedRect(vg, bx, by, bw, bh, cr);
        nvgStrokeWidth(vg, theme.metrics.border_width);
        nvgStrokeColor(vg, nvgRGBA(theme.colors.border.r, theme.colors.border.g,
                                   theme.colors.border.b, theme.colors.border.a));
        nvgStroke(vg);
    }
}

// ––––––––[ handleEvent ] images are non-interactive by default.
EventResult Image::handleEvent(const Event&, const Window&) {
    return EventResult::Ignored;
}

// ––––––––[ setProperty ] supports "file", "scale_mode".
void Image::setProperty(std::string_view name, const YamlNode& value) {
    if (name == "file") {
        setImage(value.asString());
        return;
    }
    if (name == "scale_mode") {
        const std::string mode = value.asString();
        if (mode == "none")    scale_mode_ = ScaleMode::None;
        if (mode == "fit")     scale_mode_ = ScaleMode::Fit;
        if (mode == "fill")    scale_mode_ = ScaleMode::Fill;
        if (mode == "stretch") scale_mode_ = ScaleMode::Stretch;
        return;
    }
    if (name == "border") { border_ = value.asBool(); return; }
    Widget::setProperty(name, value);
}

}  // namespace fluffy
