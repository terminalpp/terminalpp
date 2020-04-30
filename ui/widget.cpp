#include "widget.h"

#include "renderer.h"

namespace ui {

    Widget::~Widget() {
#ifndef NDEBUG
        if (parent_ != nullptr) {
            LOG() << "Widget must be detached from its parent before it is deleted";
            std::terminate();
        }
#endif
        Renderer::CancelUserEvents(this);
    }


    void Widget::repaint() {
        // atomically flip the pending repaint flag
        if (pendingRepaint_.exchange(true))
            return;
        // if the flip was successful (i.e. no pending repaint so far), inform the renderer to repaint 
        sendEvent([this](){
            UI_THREAD_CHECK;
            // don't do anything if renderer has been lost in the meantime
            if (renderer_ == nullptr)
                return;
            // don't do anything if the repaint has already been cleared (could be by parent's repaint)
            if (! pendingRepaint_ && ! pendingRelayout_)
                return;
            // do we repaint the widget, or its parent? 
            Widget * w = this;
            while (w->parent_ != nullptr) {
                // if the parent needs to be repainted, we can just wait for that repaint to happen and skin this one
                if (w->parent_->pendingRepaint_)
                    return;
                if (w->repaintParent_ || w->overlaid_ || w->transparent_) {
                    w = w->parent_;
                } else {
                    break;
                }
            } 
            // render the widget
            ASSERT(w->renderer_ != nullptr);
            w->pendingRepaint_.store(false);
            if (w->pendingRelayout_) {
                w->calculateLayout();
                // note that the relayout could have invalidated the repaint, or there could be new repaint scheduled while the relayouting was done, in which case the current repaint can be ignored
                if (w->pendingRepaint_)
                    return;
            }
            // paint the widget on the buffer
            Canvas canvas{w};
            w->paint(canvas);
            // tell the renderer to redraw the the widget
            w->renderer_->repaint(w);
        });
    }

    bool Widget::focused() const {
        UI_THREAD_CHECK;
        //std::lock_guard<std::mutex> g{mRenderer_};
        return (renderer_ == nullptr) ? false : (renderer_->keyboardFocus() == this);
    }

    void Widget::sendEvent(std::function<void(void)> handler) {
        //std::lock_guard<std::mutex> g{mRenderer_};
        //if (renderer_ != nullptr)
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
//        {
//            std::lock_guard<std::mutex> g{mRenderer_};
        renderer_ = renderer;
  //      }
        renderer->widgetAttached(this);
    }

    void Widget::detachRenderer() {
        UI_THREAD_CHECK;
        ASSERT(renderer() != nullptr);
        ASSERT(parent_ == nullptr || parent_->renderer() != nullptr);
        renderer()->widgetDetached(this);
//        {
//            std::lock_guard<std::mutex> g{mRenderer_};
        renderer_ = nullptr;
//        }
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