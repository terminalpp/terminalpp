#include "canvas.h"
#include "widget.h"
#include "renderer.h"

namespace ui {

    // Renderer ===================================================================================

#ifndef NDEBUG
    std::thread::id Renderer::UIThreadId_;
#endif

    std::deque<std::pair<std::function<void(void)>, Widget*>> Renderer::UserEvents_;
    std::mutex Renderer::M_;
    std::function<void(void)> Renderer::UserEventScheduler_;

    // TODO how to deal with invalid widgets? 

    void Renderer::render(Widget * widget) {
        UI_THREAD_CHECK;
        while ((widget->overlay() != Widget::Overlay::No) && widget->parent() != nullptr)
            widget = widget->parent();
        Canvas canvas(widget); 
        widget->paint(canvas);
        // clear the repaint request flag
        widget->repaintRequested_.store(false);
        // actually render the updated part of the buffer
        render(widget->visibleRect_);
    }

    // LocalRenderer ==============================================================================

    size_t LocalRenderer::MouseClickMaxDuration_ = 200;
    size_t LocalRenderer::MouseDoubleClickMaxDistance_ = 200;


} // namespace ui