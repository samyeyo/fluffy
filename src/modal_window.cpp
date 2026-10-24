// src/modal_window.cpp

// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

// ModalWindow — semi-transparent overlay with centered dialog.

#include <fluffy/modal_window.hpp>
#include <fluffy/container.hpp>
#include <fluffy/layout.hpp>
#include <fluffy/theme.hpp>
#include <fluffy/window.hpp>
#include <nanovg.h>
namespace fluffy {
ModalWindow::ModalWindow()
    : Widget(Rect{}) {
    modal_visible_ = false;
}
void ModalWindow::setContent(std::unique_ptr<Widget> content) {
    if (!content) return;
    content->setWindow(this->window_);
    content_ = std::move(content);
}
void ModalWindow::draw(NVGcontext* vg, const Theme& theme, const Window& w) {
    if (!modal_visible_ || !visible_) return;
    const auto [dw, dh] = w.designSize();
    const float canvas_w = static_cast<float>(dw);
    const float canvas_h = static_cast<float>(dh);
    nvgBeginPath(vg);
    nvgRect(vg, 0.0f, 0.0f, canvas_w, canvas_h);
    const auto& bg = theme.colors.background;
    NVGcolor overlay_color = nvgRGBA(bg.r, bg.g, bg.b,
                                     static_cast<unsigned char>(kOverlayAlpha * 255.0f));
    nvgFillColor(vg, overlay_color);
    nvgFill(vg);
    if (content_ && content_->visible()) {
        const float cw = content_->width()  > 0.0f ? content_->width()  : kDialogMinWidth;
        const float ch = content_->height() > 0.0f ? content_->height() : kDialogMinHeight;
        const float cx = (canvas_w - cw) * 0.5f;
        const float cy = (canvas_h - ch) * 0.5f;
        content_->setBounds(Rect(cx, cy, cw, ch));
        content_->draw(vg, theme, w);
    }
}
EventResult ModalWindow::handleEvent(const Event& e, const Window& w) {
    if (!modal_visible_ || !visible_ || !enabled_) return EventResult::Ignored;
    if (auto* md = std::get_if<MouseDownEvent>(&e)) {
        if (content_) {
            const auto [dw, dh] = w.designSize();
            const float canvas_w = static_cast<float>(dw);
            const float canvas_h = static_cast<float>(dh);
            const float cw = content_->width()  > 0.0f ? content_->width()  : kDialogMinWidth;
            const float ch = content_->height() > 0.0f ? content_->height() : kDialogMinHeight;
            const float cx = (canvas_w - cw) * 0.5f;
            const float cy = (canvas_h - ch) * 0.5f;
            const Rect dlg = Rect(cx, cy, cw, ch);
            if (!dlg.containsPoint(md->x, md->y)) {
                hide();
            } else {
                content_->handleEvent(e, w);
            }
        } else {
            hide();
        }
        return EventResult::Consumed;
    }
    if (std::get_if<MouseUpEvent>(&e)) {
        if (content_) content_->handleEvent(e, w);
        return EventResult::Consumed;
    }
    if (std::get_if<MouseMoveEvent>(&e)) {
        if (content_) {
            float mx = 0.0f, my = 0.0f;
            if (window_) window_->cursorPosition(mx, my);
            content_->updateHover(mx, my);
        }
        return EventResult::Consumed;
    }
    if (std::get_if<KeyDownEvent>(&e)) {
        return EventResult::Consumed;
    }
    return EventResult::Consumed;
}
void ModalWindow::setWindow(Window* w) noexcept {
    Widget::setWindow(w);
    if (content_) content_->setWindow(w);
}
}  

