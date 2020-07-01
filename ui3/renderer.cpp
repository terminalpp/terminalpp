#include "widget.h"

#include "renderer.h"

namespace ui3 {

    void Renderer::detachWidget(Widget * widget) {
        {
            // detach the visible area
            std::lock_guard<std::mutex> g{widget->rendererGuard_};
            widget->visibleArea_.detach();
        }
        for (Widget * child : widget->children_)
            detachWidget(child);
        widgetDetached(widget);
    }

    // Layouting and painting

    void Renderer::paint(Widget * widget) {
        if (renderWidget_ == nullptr)
            renderWidget_ = widget;
        else
            renderWidget_ = renderWidget_->commonParentWith(widget);
        ASSERT(renderWidget_ != nullptr);
        // if fps is 0, render immediately, otherwise wait for the renderer to paint
        if (fps_ == 0) 
            paintAndRender();
    }

    void Renderer::paintAndRender() {
        if (renderWidget_ == nullptr)
            return;
        // lock the buffer in priority mode and paint the widget (this also clears the pending repaint flags)
        std::lock_guard<PriorityLock> g{bufferLock_.priorityLock(), std::adopt_lock};
        renderWidget_->paint();
        // render the visible area of the widget, still under the priority lock
        render(buffer_, renderWidget_->visibleArea_.bufferRect());
        renderWidget_ = nullptr;
    }   

    void Renderer::startRenderer() {
        if (renderer_.joinable())
            renderer_.join();
        renderer_ = std::thread([this](){
            while (fps_ != 0) {
                schedule([this](){
                    paintAndRender();
                });
                std::this_thread::sleep_for(std::chrono::milliseconds(1000 / fps_));
            }
        });
    }

} // namespace ui3