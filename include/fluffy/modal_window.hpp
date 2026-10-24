// include/fluffy/modal_window.hpp

// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

// ModalWindow — semi-transparent overlay with centred content.

#pragma once
#include <fluffy/geom.hpp>
#include <fluffy/theme.hpp>
#include <fluffy/widget.hpp>
#include <memory>
namespace fluffy {
class ModalWindow final : public Widget {
public:
    ModalWindow();
    ~ModalWindow() override = default;
    void show() noexcept { modal_visible_ = true; }
    void hide() noexcept { modal_visible_ = false; }
    bool isModalVisible() const noexcept { return modal_visible_; }
    void setContent(std::unique_ptr<Widget> content);
    void        draw(NVGcontext* vg, const Theme& theme, const Window& w) override;
    EventResult handleEvent(const Event& e, const Window& w) override;
    void setWindow(Window* w) noexcept override;
private:
    std::unique_ptr<Widget> content_;
    bool                    modal_visible_ = false;
    static constexpr float  kOverlayAlpha   = 0.45f;
    static constexpr float  kDialogMinWidth = 260.0f;
    static constexpr float  kDialogMinHeight = 120.0f;
};
}  

