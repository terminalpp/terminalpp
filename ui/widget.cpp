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

    void Widget::render() {
        UI_THREAD_CHECK;
        ASSERT(renderer_ != nullptr);
        pendingRepaint_.store(false);
        // paint the widget on the buffer
        Canvas canvas{this};
        paint(canvas);
        // tell the renderer to redraw the the widget
        renderer_->repaint(this);
    }

    void Widget::repaint() {
        // atomically flip the pending repaint flag, if repaint is already pending, do nothing
        if (pendingRepaint_.exchange(true))
            return;
        // if the flip was successful (i.e. no pending repaint so far), inform the renderer to repaint 
        sendEvent([this](){
            UI_THREAD_CHECK;
            // if the widget has been detached in the meantime, ignore the event
            if (renderer_ == nullptr)
                return;
            // propagate the paint event all the way to the root element, stopping if necessary
            Widget * w = this;
            Widget * sender = this;
            Widget * target = this;
            while (true) {
                ASSERT(w->renderer_ != nullptr);
                // if there is relayout scheduled on current widget, relayout
                if (w->pendingRelayout_) {
                    w->pendingRepaint_.store(false); // WHY repaint??? 
                    w->calculateLayout();
                }
                if (w != sender) {
                    // if current widget is not the actual target and is scheduled to be repainted, we can stop this request as the target child will be eventually repainted as well
                    if (w->pendingRepaint_)
                        return;
                    // translate the target
                    target = w->propagatePaintTarget(sender, target);
                    // if the repaint has been stopped, do nothing
                    if (target == nullptr)
                        return;
                }
                if (w->parent_ == nullptr)
                    break;
                sender = w;
                w = w->parent_;
            }
            target->render();
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