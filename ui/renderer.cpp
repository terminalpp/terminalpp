#include "widget.h"

#include "mixins/selection_owner.h"

#include "renderer.h"

namespace ui {

    size_t & EventQueue::PendingEvents(Widget * w) {
        return w->pendingEvents_;
    }

    Renderer::Renderer(Size const & size, EventQueue * eq):
        eq_{eq},
        eventDummy_{new Widget()},
        buffer_{size} {
    }


    Renderer::~Renderer() {
        if (fpsThread_.joinable()) {
            fps_ = 0;
            fpsThread_.join();
        }
        eq_->cancelEvents(eventDummy_);
        delete eventDummy_;
        delete eq_;
        ASSERT(root_ == nullptr) << "Deleting renderer with attached widgets is an error.";
    }

    // Events

    void Renderer::yieldToUIThread() {
        std::unique_lock<std::mutex> g{yieldGuard_};
        schedule([this](){
            std::lock_guard<std::mutex> g{yieldGuard_};
            yieldCv_.notify_all();
        });
        yieldCv_.wait(g);
    }

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
            // also make sure that the modal root is the new root
            modalRoot_ = root_;
        }
    }

    void Renderer::setModalRoot(Widget * widget) {
        ASSERT(widget->renderer() == this);
        if (modalRoot_ == widget)
            return;
        modalRoot_ = widget;
        // if we are returning back to non-modal state (modal root is the root widget itself), restore the keyboard focus
        if (widget == root_) {
            setKeyboardFocus(nonModalFocus_ != nullptr ? nonModalFocus_ : nextKeyboardFocus());
        } else {
            nonModalFocus_ = keyboardFocus_;
            setKeyboardFocus(nextKeyboardFocus());
        }

    }

    void Renderer::widgetDetached(Widget * widget) {
        if (renderWidget_ == widget)
            renderWidget_ = nullptr;
        // check mouse and keyboard focus and emit proper events if disabled
        if (widget == mouseFocus_) {
            Event<void>::Payload p{};
            widget->mouseOut(p);
            mouseFocus_ = nullptr;
        }
        if (keyboardFocus_ == widget) {
            Event<void>::Payload p{};
            widget->focusOut(p);
            keyboardFocus_ = nullptr;
        }
        // if there are pending clipboard or selection requests to the widget, stop them
        if (clipboardRequestTarget_ == widget)
            clipboardRequestTarget_ = nullptr;
        if (selectionRequestTarget_ == widget)
            selectionRequestTarget_ = nullptr;
        // cancel all user events pending on the widget
        eq_->cancelEvents(widget);
    }


    void Renderer::detachWidget(Widget * widget) {
        // block repainting of detached widgets - will be repainted again after being reattached
        widget->pendingRepaint_ = true;
        // do the bookkeeping for widget detachment before we actually touch the widget
        widgetDetached(widget);
        // detach the children 
        for (Widget * child : widget->children_)
            detachWidget(child);
        // and finally detach the visible area 
        std::lock_guard<std::mutex> g{widget->rendererGuard_};
        widget->visibleArea_.detach();
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

    void Renderer::setKeyboardFocus(Widget * widget) {
        ASSERT(widget == nullptr || ((widget->renderer() == this) && widget->focusable() && widget->enabled()));
        if (widget == keyboardFocus_ || ! widget->isDominatedBy(modalRoot_))
            return;
        // if the focus is active and different widget was focused, trigger the focusOut - if renderer is not focused, focusOut has been triggered at renderer defocus
        if (keyboardFocus_ != nullptr && focusIn_) {
            // first clear the cursor set by the old element
            buffer_.setCursor(buffer_.cursor().setVisible(false), Point{-1,-1});
            Event<void>::Payload p{};
            keyboardFocus_->focusOut(p);
        }
        // update the keyboard focus and if renderer is focused, trigger the focus in event - otherwise it will be triggered when the renderer is focused. 
        keyboardFocus_ = widget;
        if (keyboardFocus_ != nullptr && focusIn_) {
            Event<void>::Payload p{};
            keyboardFocus_->focusIn(p);
        }
    }

    /**
     */
    Widget * Renderer::nextKeyboardFocus() {
        // if some widget is already focused, start there
        if (keyboardFocus_ != nullptr) {
            Widget * result = keyboardFocus_->nextWidget(Widget::IsAvailable);
            while (result != nullptr) {
                if (result->focusable() && result->isDominatedBy(modalRoot_))
                    return result;
                result = result->nextWidget(Widget::IsAvailable);
            }
        }
        // if we get here, it means that either nothing is focused, or there is nothing that can be focused after currently focused widget. Either way, we start from the beginning
        Widget * result = modalRoot_->nextWidget(Widget::IsAvailable, nullptr, false);
        while (result != nullptr) {
            if (result->focusable() && result->isDominatedBy(modalRoot_))
                return result;
            result = result->nextWidget(Widget::IsAvailable);
        }
        return nullptr;
    }

    Widget * Renderer::prevKeyboardFocus() {
        // if some widget is already focused, start there
        if (keyboardFocus_ != nullptr) {
            Widget * result = keyboardFocus_->prevWidget(Widget::IsAvailable);
            while (result != nullptr) {
                if (result->focusable() && result->isDominatedBy(modalRoot_))
                    return result;
                result = result->prevWidget(Widget::IsAvailable);
            }
        }
        // if we get here, it means that either nothing is focused, or there is nothing that can be focused before currently focused widget. Either way, we start from the beginning (end)
        Widget * result = modalRoot_->prevWidget(Widget::IsAvailable, nullptr, false);
        while (result != nullptr) {
            if (result->focusable() && result->isDominatedBy(modalRoot_))
                return result;
            result = result->prevWidget(Widget::IsAvailable);
        }
        return nullptr;
    }

    void Renderer::focusIn() {
        ASSERT(!focusIn_);
        focusIn_ = true;
        {
            VoidEvent::Payload p{};
            onFocusIn(p, this);
        }
        // if we have focused widget, refocus it
        if (keyboardFocus_ != nullptr) {
            ui::VoidEvent::Payload p{};
            keyboardFocus_->focusIn(p);
        }
    }

    void Renderer::focusOut() {
        ASSERT(focusIn_);
        focusIn_ = false;
        if (keyboardFocus_ != nullptr) {
            ui::VoidEvent::Payload p{};
            keyboardFocus_->focusOut(p);
        }
        {
            VoidEvent::Payload p{};
            onFocusOut(p, this);
        }
    }

    void Renderer::keyDown(Key k) {
        ASSERT(focusIn_);
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
        ASSERT(focusIn_);
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
        ASSERT(focusIn_);
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
        ASSERT(! mouseIn_);
        mouseIn_ = true;
        mouseButtons_ = 0;
        mouseFocus_ = nullptr;
        // trigger mouseIn event
        VoidEvent::Payload p{};
        onMouseIn(p, this);
    }

    void Renderer::mouseOut() {
        ASSERT(mouseIn_);
        if (mouseFocus_ != nullptr) {
            ui::VoidEvent::Payload p{};
            mouseFocus_->mouseOut(p);
        }
        mouseIn_ = false;
        mouseButtons_ = 0;
        mouseFocus_ = nullptr;
    }

    void Renderer::mouseMove(Point coords) {
        ASSERT(mouseIn_);
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
        ASSERT(mouseIn_);
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
        ASSERT(mouseIn_);
        updateMouseFocus(coords);
        // if this is first button to be held down, set the click start time and click button, otherwise invalidate the click info (multiple pressed buttons don't register as click)
        if (mouseButtons_ == 0) {
            mouseClickStart_ = SteadyClockMillis();
            mouseClickButton_ = static_cast<unsigned>(button);
        } else {
            mouseClickButton_ = 0;
            lastMouseClickTarget_ = nullptr;
        }
        // update the mouse buttons and emit the event, first own and then the target widget's
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
        ASSERT(mouseIn_ && mouseButtons_ != 0);
        // first emit the mouse up event, first by the renderer and then by the mouse target
        ASSERT(mouseFocus_ != nullptr);
        if (onMouseUp.attached()) {
            MouseButtonEvent::Payload p{coords, button, modifiers_};
            onMouseUp(p, this);
            // if the mouse up was stopped at the renderer's event, invalidate the click and double click bookkeeping 
            if (!p.active()) {
                mouseButtons_ &= static_cast<unsigned>(button);
                mouseClickButton_ = 0;
                lastMouseClickTarget_ = nullptr;
                return;
            }
        }
        if (mouseFocus_ != nullptr) {
            ui::MouseButtonEvent::Payload p{mouseFocus_->toWidgetCoordinates(coords), button, modifiers_};
            mouseFocus_->mouseUp(p);
        }
        // determine if we are dealing with a click, or a double click
        if (mouseButtons_ == static_cast<unsigned>(button)) {
            size_t now = SteadyClockMillis();
            if (now - mouseClickStart_ <= mouseClickMaxDuration_) {
                if (
                    (lastMouseClickTarget_ == mouseFocus_) &&
                    (lastMouseClickButton_ == static_cast<unsigned>(button)) &&
                    (mouseClickStart_ - lastMouseClickEnd_ < mouseDoubleClickMaxDistance_)
                ) {
                    // emit the double click event
                    mouseDoubleClick(coords, button);
                    // reset the double click state by clearing the widget and button
                    lastMouseClickTarget_ = nullptr;
                    lastMouseClickButton_ = 0;
                // not a double click, emit single click
                } else {
                    mouseClick(coords, button);
                    // update the double click state
                    lastMouseClickEnd_ = now;
                    lastMouseClickButton_ = static_cast<unsigned>(button);
                    lastMouseClickTarget_ = mouseFocus_;
                }
            }
            mouseClickButton_ = 0;
            lastMouseClickTarget_ = nullptr;
        // if not click or double click, update the mouse buttons and clear the click and double click state
        } else {
            mouseButtons_ &= static_cast<unsigned>(button);
            mouseClickButton_ = 0;
            lastMouseClickTarget_ = nullptr;
        }
    }

    void Renderer::mouseClick(Point coords, MouseButton button) {
        ASSERT(mouseIn_);
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
        ASSERT(mouseIn_);
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
                ui::VoidEvent::Payload p{};
                mouseFocus_->mouseOut(p);
            }
            mouseFocus_ = newTarget;
            if (mouseFocus_ != nullptr) {
                ui::VoidEvent::Payload p{};
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