#include "canvas.h"
#include "widget.h"
#include "renderer.h"

namespace ui2 {


    // TODO how to deal with invalid widgets? 

    void Renderer::paintWidget(Widget * widget) {
        UI_THREAD_CHECK;
        while (widget->isOverlaid() && widget->parent() != nullptr)
            widget = widget->parent();
        Canvas canvas(widget); 
        widget->paint(canvas);
        // actually render the updated part of the buffer
        render(widget->visibleRect_);
    }


} // namespace ui2