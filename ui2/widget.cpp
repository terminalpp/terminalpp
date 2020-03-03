#include "widget.h"

#include "renderer.h"

namespace ui2 {

    void Widget::repaint() {
        Renderer * r = renderer();
        if (r != nullptr)
            r->repaint(this);
    }

    bool Widget::focused() const {
        UI_THREAD_CHECK;
        Renderer * r = renderer();
        return (r == nullptr) ? false : (r->keyboardFocus() == this);
    }


    void Widget::attachRenderer(Renderer * renderer) {
        UI_THREAD_CHECK;
        ASSERT(this->renderer() == nullptr && renderer != nullptr);
        ASSERT(parent_ == nullptr || parent_->renderer() == renderer);
        renderer_.store(renderer);
        renderer->widgetAttached(this);
    }

    void Widget::detachRenderer() {
        UI_THREAD_CHECK;
        ASSERT(renderer() != nullptr);
        ASSERT(parent_ == nullptr || parent_->renderer() != nullptr);
        renderer()->widgetDetached(this);
        renderer_.store(nullptr);
    }

} // namespace ui