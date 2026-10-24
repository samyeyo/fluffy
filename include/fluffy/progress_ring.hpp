// include/fluffy/progress_ring.hpp
// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
// ProgressRing — indeterminate spinner / determinate arc
// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
#pragma once
#include <fluffy/widget.hpp>
namespace fluffy {
class ProgressRing final : public Widget {
public:
    explicit ProgressRing(Rect bounds = Rect{}, float value = -1.0f);

    // ––––––––[ setValue ] -1 = indeterminate, 0..1 determinate
    void setValue(float v);

    // ––––––––[ setIndeterminate ] true = spinner
    void setIndeterminate(bool b);

    float value() const noexcept { return value_; }

    void draw(NVGcontext* vg, const Theme& theme, const Window& w) override;
    EventResult handleEvent(const Event& e, const Window& w) override;
    void tick(float dt, const Theme& theme) noexcept override;
    void setProperty(std::string_view name, const YamlNode& value) override;
    float naturalWidth(const Theme& theme, const Window& w) const override;
    float naturalHeight(const Theme& theme, const Window& w) const override;

private:
    // ––––––––[ value_ ] -1 indeterminate, else 0..1
    float value_ = -1.0f;
    // ––––––––[ angle_ ] spinner angle
    float angle_ = 0.0f;
};
}  // namespace fluffy
