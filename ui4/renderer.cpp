#include "renderer.h"
#include "widget.h"

namespace ui {

    void Renderer::setFps(unsigned value) {
        ASSERT(IN_UI_THREAD);
        if (fps_ == value)
            return;
        if (fps_ == REPAINT_IMMEDIATE) {
            fps_ = value;
            // start the periodic fps thread, first wait for the thread in case it is dying 
            if (fpsThread_.joinable())
                fpsThread_.join();
            // start the thread
            fpsThread_ = std::thread([this](){
                while (fps_ != REPAINT_IMMEDIATE) {
                    Widget::Schedule([this]() {
                        ASSERT(IN_UI_THREAD);
                        if (repaintRoot_ != nullptr) {
                            // repaint the repaint root
                            repaint(repaintRoot_);
                            repaintRoot_ = nullptr;
                        }
                    });
                    std::this_thread::sleep_for(std::chrono::milliseconds(1000 / fps_));
                }
            });
        } else {
            fps_ = value; 
        }
    }

    void Renderer::updateWidget(Widget * widget) {
        ASSERT(IN_UI_THREAD);
        ASSERT(widget->renderer_ == this);
        if (fps_ == REPAINT_IMMEDIATE) {
            repaint(widget);
        } else {
            repaintRoot_ = Widget::CommonParent(repaintRoot_, widget);
        }
    }

    void Renderer::repaint(Widget * widget) {
        ASSERT(IN_UI_THREAD);
        ASSERT(widget->renderer_ == this);
        Widget::Repaint(widget);
    }


} // namespace ui