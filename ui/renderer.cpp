#include "widget.h"

#include "mixins/selection_owner.h"

#include "renderer.h"

namespace ui {

    Renderer::Renderer(Size const & size, EventQueue & eq):
        eq_{eq},
        eventDummy_{new Widget()},
        buffer_{size} {
    }


    Renderer::~Renderer() {
        if (fpsThread_.joinable()) {
            fps_ = 0;
            fpsThread_.join();
        }
        eq_.cancelEvents(eventDummy_);
        delete eventDummy_;
        ASSERT_PANIC(root_ == nullptr) << "Deleting renderer with attached widgets is an error.";
    }

    // Events

    void Renderer::yieldToUIThread() {
        std::unique_lock<std::mutex> g{yieldGuard_};
        bool yielded = false;
        schedule([this, & yielded](){
            std::lock_guard<std::mutex> g{yieldGuard_};
            yielded = true;
            yieldCv_.notify_all();
        });
        while (!yielded)
            yieldCv_.wait(g);
    }

    // Widget Tree

    void Renderer::setRoot(Widget * value) {
        UI_THREAD_ONLY;
        if (root_ != value) {
            if (root_ != nullptr)
                detachTree(root_);
            root_ = value;
            if (value != nullptr) {
                root_->renderer_ = this;
                // either resize, or just relayout, which triggers repaint and propagates the visible area update to all children
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
        UI_THREAD_ONLY;
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
        eq_.cancelEvents(widget);
    }


    void Renderer::detachWidget(Widget * widget) {
        // block repainting of detached widgets - will be repainted again after being reattached
        widget->requests_.fetch_or(Widget::REQUEST_REPAINT);
        // do the bookkeeping for widget detachment before we actually touch the widget
        widgetDetached(widget);
        // detach the children 
        for (Widget * child : widget->children_)
            detachWidget(child);
        // and finally detach the visible area 
        std::lock_guard<std::mutex> g{widget->rendererGuard_};
        widget->renderer_ = nullptr;
        widget->visibleArea_ = Canvas::VisibleArea{};
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
        UI_THREAD_ONLY;
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
        UI_THREAD_ONLY;
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
            p.updateSender(keyboardFocus_);
            keyboardFocus_->focusIn(p);
        }
    }

    void Renderer::focusOut() {
        ASSERT(focusIn_);
        focusIn_ = false;
        if (keyboardFocus_ != nullptr) {
            ui::VoidEvent::Payload p{};
            p.updateSender(keyboardFocus_);
            keyboardFocus_->focusOut(p);
        }
        {
            // when renderer is focused out, reset the modifiers so that new focus starts with clean state
            modifiers_ = Key::None;
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
            p.updateSender(keyboardFocus_);
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
            p.updateSender(keyboardFocus_);
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
            p.updateSender(keyboardFocus_);
            keyboardFocus_->keyChar(p);
        }
    }

    // Mouse Input

    void Renderer::mouseIn() {
        ASSERT(! mouseIn_);
        mouseIn_ = true;
        mouseButtons_ = 0;
        mouseFocus_ = nullptr;
        // reset mouse cursor
        setMouseCursor(MouseCursor::Default);
        // trigger mouseIn event
        VoidEvent::Payload p{};
        onMouseIn(p, this);
    }

    void Renderer::mouseOut() {
        ASSERT(mouseIn_);
        if (mouseFocus_ != nullptr) {
            ui::VoidEvent::Payload p{};
            p.updateSender(mouseFocus_);
            mouseFocus_->mouseOut(p);
        }
        mouseIn_ = false;
        mouseButtons_ = 0;
        mouseFocus_ = nullptr;
        mouseCoords_ = Point{-1, -1};
        VoidEvent::Payload p{};
        onMouseOut(p, this);
    }

    void Renderer::mouseMove(Point coords) {
        ASSERT(mouseIn_);
        if (coords == mouseCoords_)
            return;
        mouseCoords_ = coords;
        updateMouseFocus(coords);
        if (onMouseMove.attached()) {
            MouseMoveEvent::Payload p{coords, modifiers_};
            onMouseMove(p, this);
            if (!p.active())
                return;
        }
        if (mouseFocus_ != nullptr) {
            ui::MouseMoveEvent::Payload p{mouseFocus_->toWidgetCoordinates(coords), modifiers_};
            p.updateSender(mouseFocus_);
            mouseFocus_->mouseMove(p);
        }
    }

    void Renderer::mouseWheel(Point coords, int by) {
        ASSERT(mouseIn_);
        mouseCoords_ = coords;
        updateMouseFocus(coords);
        if (onMouseWheel.attached()) {
            MouseWheelEvent::Payload p{coords, by, modifiers_};
            onMouseWheel(p, this);
            if (!p.active())
                return;
        }
        if (mouseFocus_ != nullptr) {
            ui::MouseWheelEvent::Payload p{mouseFocus_->toWidgetCoordinates(coords), by, modifiers_};
            p.updateSender(mouseFocus_);
            mouseFocus_->mouseWheel(p);
        }
    }

    void Renderer::mouseDown(Point coords, MouseButton button) {
        ASSERT(mouseIn_);
        mouseCoords_ = coords;
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
            p.updateSender(mouseFocus_);
            mouseFocus_->mouseDown(p);
        }
    }

    void Renderer::mouseUp(Point coords, MouseButton button) {
        ASSERT(mouseIn_ && mouseButtons_ != 0);
        // first emit the mouse up event, first by the renderer and then by the mouse target
        ASSERT(mouseFocus_ != nullptr);
        mouseCoords_ = coords;
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
            p.updateSender(mouseFocus_);
            mouseFocus_->mouseUp(p);
        }
        emitMouseClicks(coords, button);
        mouseClickButton_ = 0;
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
            p.updateSender(mouseFocus_);
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
            p.updateSender(mouseFocus_);
            mouseFocus_->mouseDoubleClick(p);
        }
    }

    void Renderer::mouseTripleClick(Point coords, MouseButton button) {
        ASSERT(mouseIn_);
        if (onMouseTripleClick.attached()) {
            MouseButtonEvent::Payload p{coords, button, modifiers_};
            onMouseTripleClick(p, this);
            if (!p.active())
                return;
        }
        if (mouseFocus_ != nullptr) {
            ui::MouseButtonEvent::Payload p{mouseFocus_->toWidgetCoordinates(coords), button, modifiers_};
            p.updateSender(mouseFocus_);
            mouseFocus_->mouseTripleClick(p);
        }
    }

    void Renderer::emitMouseClicks(Point coords, MouseButton button) {
        if (mouseButtons_ == static_cast<unsigned>(button)) {
            mouseButtons_ = 0;
            size_t now = SteadyClockMillis();
            if (now - mouseClickStart_ <= mouseClickMaxDuration_) {
                // if the last mouse click target is identical to current mouse target and the button is the same as well, we might have double or triple click
                if (lastMouseClickTarget_ != nullptr && 
                    lastMouseClickTarget_ == mouseFocus_ &&
                    lastMouseClickButton_ == static_cast<unsigned>(button)
                ) {
                    if (mouseClickStart_ - lastMouseDoubleClickEnd_ < mouseDoubleClickMaxDistance_) {
                        mouseTripleClick(coords, button);
                        // reset the counters by setting mouse click target to nullptr and double click end outside of the range
                        lastMouseClickTarget_ = nullptr;
                        lastMouseClickButton_ = 0;
                        lastMouseDoubleClickEnd_ = now - mouseDoubleClickMaxDistance_ - 1;
                        return;
                    } else if (mouseClickStart_ - lastMouseClickEnd_ < mouseDoubleClickMaxDistance_) {
                        mouseDoubleClick(coords, button);
                        lastMouseDoubleClickEnd_ = now;
                        lastMouseClickEnd_ = now - mouseDoubleClickMaxDistance_ - 1;
                        return;
                    } 
                } 
                // otherwise we are dealing with a single click
                mouseClick(coords, button);
                // update the double click state
                lastMouseClickEnd_ = now;
                lastMouseClickButton_ = static_cast<unsigned>(button);
                lastMouseClickTarget_ = mouseFocus_;
            }
        } else {
            mouseButtons_ &= ~ static_cast<unsigned>(button);
            lastMouseClickTarget_ = nullptr;
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
                NOT_IMPLEMENTED;
                //dynamic_cast<SelectionOwner*>(owner)->clearSelection();
        }
    }



} // namespace ui