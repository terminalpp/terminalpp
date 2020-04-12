#include "canvas.h"
#include "widget.h"
#include "renderer.h"

namespace ui {

    // Renderer ===================================================================================

    std::deque<std::function<void(void)>> Renderer::UserEvents_;
    std::mutex Renderer::M_;
    std::function<void(void)> Renderer::UserEventScheduler_;

    // TODO how to deal with invalid widgets? 

    void Renderer::render(Widget * widget) {
        UI_THREAD_CHECK;
        while (widget->isOverlaid() && widget->parent() != nullptr)
            widget = widget->parent();
        Canvas canvas(widget); 
        widget->paint(canvas);
        // actually render the updated part of the buffer
        render(widget->visibleRect_);
    }

    // LocalRenderer ==============================================================================

    size_t LocalRenderer::MouseClickMaxDuration_ = 200;
    size_t LocalRenderer::MouseDoubleClickMaxDistance_ = 200;


} // namespace ui