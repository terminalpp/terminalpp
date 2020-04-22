#include "widget.h"

#include "renderer.h"

namespace ui {

    void Widget::repaint() {
        // don't do anything if repaint has already been requested
        if (repaintRequested_.exchange(true)) 
            return;
        std::lock_guard<std::mutex> g{mRenderer_};
        if (renderer_ != nullptr)
            renderer_->repaint(this);
    }

    bool Widget::focused() const {
        UI_THREAD_CHECK;
        std::lock_guard<std::mutex> g{mRenderer_};
        return (renderer_ == nullptr) ? false : (renderer_->keyboardFocus() == this);
    }

    void Widget::sendEvent(std::function<void(void)> handler) {
        std::lock_guard<std::mutex> g{mRenderer_};
        if (renderer_ != nullptr)
            Renderer::SendEvent(handler, this);
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

    void Widget::setEnabled(bool value) {
        UI_THREAD_CHECK;
        if (enabled_ != value) {
            enabled_ = value;
            if (focused()) {
                ASSERT(renderer_ != nullptr);
                renderer_->setKeyboardFocus(renderer_->keyboardFocusNext());
            }
        }
    }


    void Widget::attachRenderer(Renderer * renderer) {
        UI_THREAD_CHECK;
        ASSERT(this->renderer() == nullptr && renderer != nullptr);
        ASSERT(parent_ == nullptr || parent_->renderer() == renderer);
        {
            std::lock_guard<std::mutex> g{mRenderer_};
            renderer_ = renderer;
        }
        renderer->widgetAttached(this);
    }

    void Widget::detachRenderer() {
        UI_THREAD_CHECK;
        ASSERT(renderer() != nullptr);
        ASSERT(parent_ == nullptr || parent_->renderer() != nullptr);
        renderer()->widgetDetached(this);
        {
            std::lock_guard<std::mutex> g{mRenderer_};
            renderer_ = nullptr;
        }
    }

    void Widget::requestClipboard() {
        UI_THREAD_CHECK;
        if (renderer_ != nullptr)
            renderer_->requestClipboard(this);
    }

    void Widget::requestSelection() {
        UI_THREAD_CHECK;
        if (renderer_ != nullptr)
            renderer_->requestSelection(this);
    }

    void Widget::setClipboard(std::string const & contents) {
        UI_THREAD_CHECK;
        if (renderer_ != nullptr)
            renderer_->rendererSetClipboard(contents);
    }

    void Widget::registerSelection(std::string const & contents) {
        UI_THREAD_CHECK;
        if (renderer_ != nullptr)
            renderer_->rendererRegisterSelection(contents, this);
    }

    /** Gives up the selection and informs the renderer. 
     */
    void Widget::clearSelection() {
        UI_THREAD_CHECK;
        if (hasSelectionOwnership()) {
            ASSERT(renderer_ != nullptr);
            renderer_->rendererClearSelection();
        }
    }

    /** Returns true if the widget currently holds the selection ownership. 
     */
    bool Widget::hasSelectionOwnership() const {
        if (renderer_ == nullptr)
            return false;
        return renderer_->selectionOwner_ == this;
    }

} // namespace ui