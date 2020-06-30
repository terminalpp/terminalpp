#include "widget.h"

#include "renderer.h"

namespace ui3 {

    void Renderer::detachWidget(Widget * widget) {
        widget->visibleArea_.detach();
        for (Widget * child : widget->children_)
            detachWidget(child);
        widgetDetached(widget);
    }

    // Layouting and painting

    void Renderer::paintAndRender(Widget * widget) {
        // lock the buffer in priority mode and paint the widget (this also clears the pending repaint flags)
        std::lock_guard<PriorityLock> g{bufferLock_.priorityLock(), std::adopt_lock};
        widget->paint();
        // render the visible area of the widget, still under the priority lock
        render(buffer_, widget->visibleArea_.bufferRect());
    }   

} // namespace ui3