#include "widget.h"

#include "mixins/selection_owner.h"

#include "renderer.h"

namespace ui {

    // Events

    /*
    void Renderer::schedule(std::function<void()> event, Widget * widget) {
        {
            std::lock_guard<std::mutex> g{eventsGuard_};
            events_.push_back(std::make_pair(event, widget));
            ++widget->pendingEvents_;
        }
    } */

    void Renderer::yieldToUIThread() {
        std::unique_lock<std::mutex> g{yieldGuard_};
        schedule([this](){
            std::lock_guard<std::mutex> g{yieldGuard_};
            yieldCv_.notify_all();
        });
        yieldCv_.wait(g);
    }

    /*
    void Renderer::processEvent() {
        std::function<void()> handler;
        {
            std::lock_guard<std::mutex> g{eventsGuard_};
            while (true) {
                if (events_.empty())
                    return;
                auto e = events_.front();
                events_.pop_front();
                if (! e.first)
                    continue;
                if (e.second != nullptr)
                    -- (e.second->pendingEvents_);
                handler = e.first;
                break;
            }
            handler();
        }
    } */

    /*
    void Renderer::cancelWidgetEvents(Widget * widget) {
        std::lock_guard<std::mutex> g{eventsGuard_};
        if (widget->pendingEvents_ == 0)
            return;
        for (auto & e : events_)
            if (e.second == widget)
                e.first = nullptr;
    }
    */

    // Widget Tree

    void Renderer::setRoot(Widget * value) {
        if (root_ != value) {
            if (root_ != nullptr)
                detachTree(root_);
            root_ = value;
            if (value != nullptr) {
                root_->visibleArea_.attach(this);
                // once attached, we can clear the repaint flag
                root_->pendingRepaint_ = false;
                // and either resize, or just relayout, which triggers repaint and propagates the visible area update to all children
                if (root_->rect().size() != size())
                    root_->resize(size());
                else
                    root_->relayout();
            }
        }
    }


    void Renderer::detachWidget(Widget * widget) {
        // block repainting of detached widgets - will be repainted again after being reattached
        widget->pendingRepaint_ = true;
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

    void Renderer::resize(Size const & value) {
        if (buffer_.size() != value) {
            buffer_.resize(value);
            // resize the root widget if any
            if (root_ != nullptr)
                root_->resize(value);
        }
    }

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
        // paint the widget on the buffer
        renderWidget_->paint();
        // render the visible area of the widget, still under the priority lock
        render(renderWidget_->visibleArea_.bufferRect());
        renderWidget_ = nullptr;
    }   

    void Renderer::startFPSThread() {
        if (fpsThread_.joinable())
            fpsThread_.join();
        fpsThread_ = std::thread([this](){
            while (fps_ != 0) {
                schedule([this](){
                    paintAndRender();
                });
                std::this_thread::sleep_for(std::chrono::milliseconds(1000 / fps_));
            }
        });
    }

    // Keyboard Input

    void Renderer::focusIn() {
        // TODO
    }

    void Renderer::focusOut() {
        // TODO
    }

    void Renderer::keyDown(Key k) {
        keyDownFocus_ = keyboardFocus_;
        modifiers_ = k.modifiers();
        if (onKeyDown.attached()) {
            KeyEvent::Payload p{k};
            onKeyDown(p, this);
            if (!p.active())
                return;
        }
        if (keyboardFocus_ != nullptr) {
            ui::KeyEvent::Payload p{k};
            keyboardFocus_->keyDown(p);
        }
    }

    void Renderer::keyUp(Key k) {
        modifiers_ = k.modifiers();
        if (onKeyUp.attached()) {
            KeyEvent::Payload p{k};
            onKeyUp(p, this);
            if (!p.active())
                return;
        }
        if (keyboardFocus_ != nullptr) {
            ui::KeyEvent::Payload p{k};
            keyboardFocus_->keyUp(p);
        }
    }

    void Renderer::keyChar(Char c) {
        if (onKeyChar.attached()) {
            KeyCharEvent::Payload p{c};
            onKeyChar(p, this);
            if (!p.active()) {
                keyboardFocus_ = nullptr;
                return;
            }
        }
        if (keyboardFocus_ == keyDownFocus_ && keyboardFocus_ != nullptr) {
            keyDownFocus_ = nullptr;
            ui::KeyCharEvent::Payload p{c};
            keyboardFocus_->keyChar(p);
        }
    }

    // Mouse Input

    void Renderer::mouseIn() {

    }

    void Renderer::mouseOut() {
        
    }

    void Renderer::mouseMove(Point coords) {
        updateMouseFocus(coords);
        if (onMouseMove.attached()) {
            MouseMoveEvent::Payload p{coords, modifiers_};
            onMouseMove(p, this);
            if (!p.active())
                return;
        }
        if (mouseFocus_ != nullptr) {
            ui::MouseMoveEvent::Payload p{mouseFocus_->toWidgetCoordinates(coords), modifiers_};
            mouseFocus_->mouseMove(p);
        }
    }

    void Renderer::mouseWheel(Point coords, int by) {
        updateMouseFocus(coords);
        if (onMouseWheel.attached()) {
            MouseWheelEvent::Payload p{coords, by, modifiers_};
            onMouseWheel(p, this);
            if (!p.active())
                return;
        }
        if (mouseFocus_ != nullptr) {
            ui::MouseWheelEvent::Payload p{mouseFocus_->toWidgetCoordinates(coords), by, modifiers_};
            mouseFocus_->mouseWheel(p);
        }
    }

    void Renderer::mouseDown(Point coords, MouseButton button) {
        updateMouseFocus(coords);
        mouseButtons_ |= static_cast<unsigned>(button);
        if (onMouseDown.attached()) {
            MouseButtonEvent::Payload p{coords, button, modifiers_};
            onMouseDown(p, this);
            if (!p.active())
                return;
        }
        if (mouseFocus_ != nullptr) {
            ui::MouseButtonEvent::Payload p{mouseFocus_->toWidgetCoordinates(coords), button, modifiers_};
            mouseFocus_->mouseDown(p);
        }
    }

    void Renderer::mouseUp(Point coords, MouseButton button) {

    }

    void Renderer::mouseClick(Point coords, MouseButton button) {
        if (onMouseClick.attached()) {
            MouseButtonEvent::Payload p{coords, button, modifiers_};
            onMouseClick(p, this);
            if (!p.active())
                return;
        }
        if (mouseFocus_ != nullptr) {
            ui::MouseButtonEvent::Payload p{mouseFocus_->toWidgetCoordinates(coords), button, modifiers_};
            mouseFocus_->mouseClick(p);
        }
    }

    void Renderer::mouseDoubleClick(Point coords, MouseButton button) {
        if (onMouseDoubleClick.attached()) {
            MouseButtonEvent::Payload p{coords, button, modifiers_};
            onMouseDoubleClick(p, this);
            if (!p.active())
                return;
        }
        if (mouseFocus_ != nullptr) {
            ui::MouseButtonEvent::Payload p{mouseFocus_->toWidgetCoordinates(coords), button, modifiers_};
            mouseFocus_->mouseDoubleClick(p);
        }
    }

    void Renderer::updateMouseFocus(Point coords) {
        // if mouse is captured to a valid mouseFocus widget, do nothing
        if (mouseButtons_ != 0 && mouseFocus_ != nullptr)
            return;
        // determine the mouse target
        Widget * newTarget = nullptr;
        if (modalRoot_ != nullptr)
            newTarget = modalRoot_->getMouseTarget(modalRoot_->toWidgetCoordinates(coords));
        // check if the target has changed and emit the appropriate events
        if (mouseFocus_ != newTarget) {
            if (mouseFocus_ != nullptr) {
                VoidEvent::Payload p{};
                mouseFocus_->mouseOut(p);
            }
            mouseFocus_ = newTarget;
            if (mouseFocus_ != nullptr) {
                VoidEvent::Payload p{};
                mouseFocus_->mouseIn(p);
            }
        }

    }

    // Selection & Clipboard

    void Renderer::pasteClipboard(std::string const & contents) {
        if (clipboardRequestTarget_ != nullptr) {
            PasteEvent::Payload p{contents, clipboardRequestTarget_};
            clipboardRequestTarget_ = nullptr;
            onPaste(p, this);
            if (! p.active())
                return;
            StringEvent::Payload pe{contents};
            p->target->paste(pe);
        }
    }

    void Renderer::pasteSelection(std::string const & contents) {
        if (selectionRequestTarget_ != nullptr) {
            PasteEvent::Payload p{contents, selectionRequestTarget_};
            selectionRequestTarget_ = nullptr;
            onPaste(p, this);
            if (! p.active())
                return;
            StringEvent::Payload pe{contents};
            p->target->paste(pe);
        }
    }

    void Renderer::clearSelection(Widget * sender) {
        if (selectionOwner_ != nullptr) {
            Widget * owner = selectionOwner_;
            selectionOwner_ = nullptr;
            // inform the sender if the request is coming from elsewhere
            if (owner != sender)
                dynamic_cast<SelectionOwner*>(owner)->clearSelection();
        }
    }



} // namespace ui