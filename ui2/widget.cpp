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

    void Widget::setCursor(Canvas & canvas, Cursor const & cursor, Point position) {
        if (focused()) {
            canvas.buffer_.setCursor(cursor);
            if (canvas.visibleRect_.contains(position))
                canvas.buffer_.setCursorPosition(position + canvas.bufferOffset_);
            else
                canvas.buffer_.setCursorPosition(Point{-1, -1});
        } else {
            ASSERT("Attempt to set cursor from unfocused widget");
        }
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