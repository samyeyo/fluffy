// include/fluffy/image.hpp

// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

// Image — displays raster images loaded via platform-native decoders.

#pragma once
#include <fluffy/widget.hpp>
#include <string>
#include <vector>

namespace fluffy {

// ––––––––[ ScaleMode ] how the image fills its bounds.
enum class ScaleMode {
    None,     // native size, no scaling
    Fit,      // preserve aspect ratio, fit inside bounds
    Fill,     // preserve aspect ratio, cover entire bounds
    Stretch   // ignore aspect ratio, fill bounds exactly
};

// ––––––––[ Image ] raster image widget using platform-native decoding.
class Image final : public Widget {
public:
    Image(Rect bounds = Rect{}, std::string_view filepath = {});
    Image(Rect bounds, int width, int height, const unsigned char* rgba);
    ~Image() override;

    Image(const Image&) = delete;
    Image& operator=(const Image&) = delete;

    void draw(NVGcontext* vg, const Theme& theme, const Window& w) override;
    EventResult handleEvent(const Event& e, const Window& w) override;
    float naturalWidth(const Theme& theme, const Window& w) const override;
    float naturalHeight(const Theme& theme, const Window& w) const override;
    void setProperty(std::string_view name, const YamlNode& value) override;

    void setImage(const std::string& filepath);
    void setImage(int w, int h, const unsigned char* rgba);
    void clearImage();

    int imageWidth() const noexcept { return img_w_; }
    int imageHeight() const noexcept { return img_h_; }
    bool hasImage() const noexcept { return image_id_ >= 0; }

    void setScaleMode(ScaleMode mode) noexcept { scale_mode_ = mode; }
    ScaleMode scaleMode() const noexcept { return scale_mode_; }

    void setBorder(bool b) noexcept { border_ = b; }
    bool hasBorder() const noexcept { return border_; }

private:
    void uploadToGPU();

    int       image_id_ = -1;
    int       img_w_    = 0;
    int       img_h_    = 0;
    ScaleMode scale_mode_ = ScaleMode::Fit;
    bool      border_     = false;
    std::vector<unsigned char> pending_rgba_;  // CPU-side until GPU upload
};

}  // namespace fluffy
