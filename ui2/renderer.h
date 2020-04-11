#pragma once

#include <deque>
#include <mutex>


#ifndef NDEBUG
#include <thread>
#endif


#include "helpers/time.h"
#include "helpers/log.h"

#include "common.h"
#include "buffer.h"
#include "widget.h"

namespace ui2 {


    /** UI Renderer
     
        Class responsible for rendering the widgets and providing the user actions such as keyboard, mouse and selection & clipboard.

        TODO

        - create the tab order of widgets and figure out how to work with it
        - and how to set the modal root and stuff
        - and the actual widget painting and resizing stuff 
        - do buffer

     */
    class Renderer {
    public:

        virtual ~Renderer() {
            // TODO delete the attached widgets            
        }

        int width() const {
            UI_THREAD_CHECK;
            return buffer_.width();
        }

        int height() const {
            UI_THREAD_CHECK;
            return buffer_.height();
        }

        Color const &  backgroundColor() const {
            return backgroundColor_;
        }

        Widget * rootWidget() const {
            return rootWidget_;
        }

        void setRootWidget(Widget * widget) {
            UI_THREAD_CHECK;
            // detach the old root widget if any
            if (rootWidget_ != nullptr) {
                rootWidget_->detachRenderer();
            }
            rootWidget_ = widget;
            // attach the new widget & resize it to the renderer
            if (rootWidget_ != nullptr) {
                rootWidget_->attachRenderer(this);
                rootWidget_->setRect(Rect::FromWH(width(), height()));
                rootWidget_->visibleRect_ = Rect::FromWH(width(), height());
                rootWidget_->bufferOffset_ = Point{0,0};
                modalRoot_ = rootWidget_;
                repaint(rootWidget_);
            }
        }

        Widget * keyboardFocus() const {
            return keyboardIn_ ? keyboardFocus_ : nullptr;
        }

        void setKeyboardFocus(Widget * widget) {
            ASSERT(widget == nullptr || widget->renderer() == this);
            if (keyboardFocus_ != nullptr && keyboardIn_) {
                Event<void>::Payload p{};
                focusOut(p, keyboardFocus_);
                // just make sure the cursor of the old widget won't be displayed
                buffer_.setCursor(buffer_.cursor().setVisible(false));
            }
            keyboardFocus_ = widget;
            if (keyboardFocus_ != nullptr && keyboardIn_) {
                Event<void>::Payload p{};
                focusIn(p, keyboardFocus_);
            }
        }

        /** Requests repaint of the given widget. [thread-safe]
         
            The purpose of this method is to use whatever event queue (or other mechanism) the target rendering supports to schedule repaint of the specified widget in the main UI thread. 
         */
        virtual void repaint(Widget * widget) = 0;

        /** Schedules an user event to be executed in the main thread. [thread-safe]
         
            The sendEvent() method provides a simple mechanism to execute arbitrary code in the main UI event loop.
         */
        static void SendEvent(std::function<void(void)> handler) {
            {
                std::lock_guard<std::mutex> g{M_};
                UserEvents_.push_back(handler);
            }
            ASSERT(UserEventScheduler_) << "UserEventScheduler not provided before user events raised";
            UserEventScheduler_();
        }

        /** Initializes the renderer by providing the scheduler function for the user events. 
         
            This is important so that user events can be scheduled even from widgets with no associated renderers, or even if no active renderers exist in the system. 
         */
        static void Initialize(std::function<void(void)> userEventScheduler) {
            ASSERT(! UserEventScheduler_) << "UserEventScheduler already specified";
            UserEventScheduler_ = userEventScheduler;
        }
        /** Executes user event and removes it from the queue.
         
            Takes the next handler from the user events queue and executes it in the main UI thread. The renderer implementation should call this function every time the main thread is informed about user event waiting to be executed. 
         */
        static void ExecuteUserEvent() {
            std::function<void(void)> handler;
            {
                if (UserEvents_.empty())
                    return;
                handler = UserEvents_.front();
                UserEvents_.pop_front();
            }
            handler();
        }

    protected:

        friend class Canvas;
        friend class Widget;

        template<typename T>
        friend class SelectionOwner;

        Renderer(int width, int height):
            buffer_{width, height},
            rootWidget_{nullptr},
            modalRoot_{nullptr},
            mouseIn_{false},
            mouseFocus_{nullptr},
            mouseButtons_{0},
            keyboardIn_{false},
            keyboardFocus_{nullptr},
            clipboardRequestTarget_{nullptr},
            selectionRequestTarget_{nullptr},
            selectionOwner_{nullptr} {
        }

        /** Called when the renderer is to be closed.

            By default the method simpy calls the rendererClose() method that actually does the closing, but overriding the behavior may add additional features, such as confirmation dialogs, etc. 
         */     
        virtual void requestClose() {
            rendererClose();
        }

        /** Closes the renderer. [thread-safe]
         
            Actually closes the renderer, no questions asked. Calling this method must eventually lead to a call to the renderer's destructor. 
         */
        virtual void rendererClose() = 0;

        /** \name Events
         
            - for keyboard best would be from 
         */
        //@{

        //@}

        /** Immediately repaints the given widget. 
         
            If the widget is overlaid with another widgets, the widget parent will be painted instead. When the painting is done, i.e. the buffer has been updated, the render method is called to actually render the update.  
         */
        void render(Widget * widget);

        /** Called when the selected rectangle of the backing buffer has been updated and needs rendered. 
         
            This method should actually render the contents. While only the refresh of the given rectangle is necessary at the time the method is called, the entire buffer is guaranteed to be valid in case whole screen repaint is to be issued. 
         */
        virtual void render(Rect const & rect) = 0;

        // ========================================================================================
        
        /** \anchor ui_renderer_mouse_input
            \name Mouse Input

            The renderer is responsible for tracking the mouse pointer and report the mouse events to the affected widgets. Similarly to other user inputs, this is split into two layers. First the trigger functions in this section are invoked directly by the renderer subclasses that monitor the user inputs. Their task is to manage the mouse state and the mouse focus widget. 

            When these functions determine that particular mouse events should be triggered at the mouse focus widget, they call the appropriate \ref ui_renderer_event_triggers "event triggers" which then raise the event at the target widget. 

            All coordinates are relative to renderer's top-left corner. 
         */
        //@{

        /** Returns the mouse buttons that are current pressed. 
         */
        size_t mouseButtonsDown() const {
            return mouseButtons_;            
        }

        /** Returns true if the mouse input is captured by the renderer. 
         
            That is either when the pointer is directly over the rendered area, or if the mouse pointer has been grabbed, such as when a button is pressed down.
         */
        bool mouseFocused() const {
            return mouseIn_;
        }

        /** Returns the widget that is current target for mouse events.
         
            This is either a widget over which the pointer is placed, or a widget that has captured the pointer. Note that the mouse focus can also be nullptr in case the mouse is not captured and outside of the renderer's window, or if the renderer does not have any widgets. 
         */
        Widget * mouseFocus() const {
            return mouseFocus_;
        }

        /** Triggered when mouse enters the rendered area.
         */
        virtual void rendererMouseIn() {
            LOG() << "MouseIn";
            ASSERT(mouseIn_ == false);
            ASSERT(mouseFocus_ == nullptr && mouseButtons_ == 0) << "Looks like mouseOut was not called properly";
            mouseFocus_ = nullptr;
            mouseButtons_ = 0;
            mouseIn_ = true;
        }

        /** Must be triggered when mouse leaves the rendered contents. 
         
            This must be the last mouse event sent to the renderer until new mouseIn call. The method should only be called
         */
        virtual void rendererMouseOut() {
            LOG() << "MouseOut";
            // trigger the mouseOut event on existing mouse focus
            Event<void>::Payload p{};
            mouseOut(p, mouseFocus_);
            // clear mouse state
            mouseFocus_ = nullptr;
            mouseButtons_ = 0;
            mouseIn_ = false;
        }

        virtual void rendererMouseMove(Point coords, Key modifiers) {
            LOG() << "Mousemove " << coords;
            ASSERT(mouseIn_);
            updateMouseFocus(coords);
            Event<MouseMoveEvent>::Payload p{MouseMoveEvent{coords, modifiers}};
            mouseMove(p, mouseFocus_);
        }

        virtual void rendererMouseWheel(Point coords, int by, Key modifiers) {
            ASSERT(mouseIn_);
            updateMouseFocus(coords);
            Event<MouseWheelEvent>::Payload p{MouseWheelEvent{coords, by, modifiers}};
            mouseWheel(p, mouseFocus_);
        }

        virtual void rendererMouseDown(Point coords, MouseButton button, Key modifiers) {
            ASSERT(mouseIn_);
            updateMouseFocus(coords);
            mouseButtons_ |= static_cast<size_t>(button);
            // dispatch the mouseDown event
            Event<MouseButtonEvent>::Payload p{MouseButtonEvent{coords, button, modifiers}};
            mouseDown(p, mouseFocus_);
        }

        /** Trigerred when mouse button is released. 
         
            The mouseUp method is forwarded to the widget which has the mouse focus (mouseFocus()). In case the last button has been released and therefore the mouse capture by the widget has been released, it is possible the mouse has moved to other widget and thefore the mouse focus transition events and the mouse move event at the target widget should be invoked. 

            These two actions are provided in separate methods so that subclasses can interject the mouse down and mouse focus move with other stuff, if needed (such as clicks detection). 
         */
        virtual void rendererMouseUp(Point coords, MouseButton button, Key modifiers) {
            rendererMouseUpEmit(coords, button, modifiers);
            // if this was the last button to be released and the capture ended, the mouse focus widget must in theory be recalculated
            if (mouseButtons_ == 0)
                rendererMouseUpUpdateTarget(coords, modifiers);
        }

        /** Actually triggers the mouse up action. 
         */
        void rendererMouseUpEmit(Point coords, MouseButton button, Key modifiers) {
            ASSERT(mouseIn_ && mouseButtons_ > 0);
            // this is technically not necessary as the mouse should be captured, but for added robustness the mouse focus is checked 
            updateMouseFocus(coords);
            mouseButtons_ &= ~ static_cast<size_t>(button);
            Event<MouseButtonEvent>::Payload p{MouseButtonEvent{coords, button, modifiers}};
            mouseUp(p, mouseFocus_);
        }

        /** Checks that given coordinates do not change the mouseFocus widget and if they do, emits the focus transition events as well as mouseMove event on the new mouse target. 
         */
        void rendererMouseUpUpdateTarget(Point coords, Key modifiers) {
            ASSERT(mouseButtons_ == 0);
            Widget * last = mouseFocus_;
            updateMouseFocus(coords);
            if (last != mouseFocus_)
                rendererMouseMove(coords, modifiers);
        }

        /** Triggered when the user clicks with the mouse.
         
            The click event is received *after* the corresponding mouse up event. If the click is also a second click in a valid double click, then only the double click event should be generated. 
         */
        virtual void rendererMouseClick(Point coords, MouseButton button, Key modifiers) {
            ASSERT(mouseIn_);
            Event<MouseButtonEvent>::Payload p{MouseButtonEvent{coords, button, modifiers}};
            mouseClick(p, mouseFocus_);
        }

        /** Triggered when the user double-click with the mouse. 
         
            A double click occurs when a single mouse button is pressed twice, sequentially within the same mouse focus target and given time period. If double-click occurs then the event should be triggered *after* the corresponding mouse up. While the first click is reported as single click, the second click is not reported and the double-click event is triggered instead. 
         */
        virtual void rendererMouseDoubleClick(Point coords, MouseButton button, Key modifiers) {
            ASSERT(mouseIn_);
            Event<MouseButtonEvent>::Payload p{MouseButtonEvent{coords, button, modifiers}};
            mouseDoubleClick(p, mouseFocus_);
        }

        /** Updates the mouse focus.
         
            The mouse focus widget is only updated if the pointer is not captured (no mouse buttons are pressed and valid mouse focus widget exists). It is possible that buttons are pressed and valid widget does not exist, such as when the widget is detached from the renderer while the buttons are pressed, in which case the mouse target is recalculated. 
         */
        virtual void updateMouseFocus(Point coords) {
            // don't change the mouse focus target if mouse is captured by a valid event
            if (mouseButtons_ != 0 && mouseFocus_ != nullptr)
                return;
            // determine the mouse target
            Widget * newTarget = (modalRoot_ != nullptr) ? modalRoot_->getMouseTarget(coords) : nullptr;
            // check if the target has changed and emit the appropriate events
            if (mouseFocus_ != newTarget) {
                if (mouseFocus_ != nullptr) {
                    Event<void>::Payload p{};
                    mouseFocus_->mouseOut(p);
                }
                mouseFocus_ = newTarget;
                if (mouseFocus_ != nullptr) {
                    Event<void>::Payload p{};
                    mouseFocus_->mouseIn(p);
                }
            }
        }

        //@}

        // ========================================================================================

        /** \anchor ui_renderer_keyboard_input
            \name Keyboard Input

         */
        //@{

        virtual void rendererFocusIn() {
            ASSERT(! keyboardIn_);
            keyboardIn_ = true;
            // focus the widget that had element last time the renderer was focused
            Event<void>::Payload p{};
            focusIn(p, keyboardFocus_);
        }

        virtual void rendererFocusOut() {
            ASSERT(keyboardIn_);
            Event<void>::Payload p{};
            focusOut(p, keyboardFocus_);
            keyboardIn_ = false;
        }

        virtual void rendererKeyChar(Char c) {
            ASSERT(keyboardIn_);
            Event<Char>::Payload p{c};
            keyChar(p, keyboardFocus_);
        }

        virtual void rendererKeyDown(Key k) {
            ASSERT(keyboardIn_);
            Event<Key>::Payload p{k};
            keyDown(p, keyboardFocus_);
        }

        virtual void rendererKeyUp(Key k) {
            ASSERT(keyboardIn_);
            Event<Key>::Payload p{k};
            keyUp(p, keyboardFocus_);
        }

        //@}

        // ========================================================================================

        /** \name Clipboard & Selection

         */
        //@{
        /** Requests the clipboard contents. 
         
            Any widget can request clipboard contents. 
         */
        virtual void requestClipboard(Widget * sender) {
            ASSERT(keyboardIn_);
            clipboardRequestTarget_ = sender;
        }

        virtual void requestSelection(Widget * sender) {
            ASSERT(keyboardIn_);
            selectionRequestTarget_ = sender;
        }

        virtual void rendererClipboardPaste(std::string const & contents) {
            Event<std::string>::Payload p{contents};
            paste(p, clipboardRequestTarget_);
            clipboardRequestTarget_ = nullptr;
        }

        virtual void rendererSelectionPaste(std::string const & contents) {
            Event<std::string>::Payload p{contents};
            paste(p, selectionRequestTarget_);
            selectionRequestTarget_ = nullptr;
        }

        /** Sets the clipboard contents. 
         
            Must be provided by the actual renderer implementation. 
         */
        virtual void rendererSetClipboard(std::string const & contents) = 0;

        /** Registers new selection contents and selection owner. 
         
            If there is already a selection owner associated with the renderer that is different from the new owner, first rendererClearSelection is called. 

            Simply updates the selection owner. 
         */
        virtual void rendererRegisterSelection(std::string const & contents, Widget * owner) {
            if (selectionOwner_ != nullptr && selectionOwner_ != owner)
                rendererClearSelection();
            selectionOwner_ = owner;
        }

        /** Clears the selection. 

            If the selection is associated with the renderer, revokes it and then tells the owner its selection has been cleared.  
         */
        virtual void rendererClearSelection() {
            if (selectionOwner_ !=  nullptr) {
                // first remove the selection from the owner, thus preventing the infinite loop calls
                Widget * owner = selectionOwner_;
                selectionOwner_ = nullptr;
                // then call clearSelection on the old renderer
                owner->clearSelection();
            }
        }

        //@}

        // ========================================================================================

        /** \anchor ui_renderer_event_triggers
            \name UI Event Triggers

            When the input processing layers (\ref ui_renderer_mouse_input "mouse") determine that particular events should be triggered in attached widgets, these dispatch methods are called with the event payloads and target widgets.  The trigger methods verify that the target is valid and the event is still active and if so, trigger the appropriate method in the target widget. 
            
            Renderer implementations may therefore override these methods to intercept (and possibly consume) any user input events globally for the entire window. 

            All coordinates are relative to renderer's top-left corner and must be converted to the target's coordinates where applicable. 

         */
        //@{
        virtual void mouseIn(Event<void>::Payload & e, Widget * target) {
            if (target != nullptr && e.active())
                target->mouseIn(e);
        }

        virtual void mouseOut(Event<void>::Payload & e, Widget * target) {
            if (target != nullptr && e.active())
                target->mouseOut(e);
        }

        virtual void mouseMove(Event<MouseMoveEvent>::Payload & e, Widget * target) {
            if (target != nullptr && e.active()) {
                e->coords = target->toWidgetCoordinates(e->coords);
                target->mouseMove(e);
            }
        }

        virtual void mouseWheel(Event<MouseWheelEvent>::Payload & e, Widget * target) {
            if (target != nullptr && e.active()) {
                e->coords = target->toWidgetCoordinates(e->coords);
                target->mouseWheel(e);
            }
        }

        virtual void mouseDown(Event<MouseButtonEvent>::Payload & e, Widget * target) {
            if (target != nullptr && e.active()) {
                e->coords = target->toWidgetCoordinates(e->coords);
                target->mouseDown(e);
            }
        }

        virtual void mouseUp(Event<MouseButtonEvent>::Payload & e, Widget * target) {
            if (target != nullptr && e.active()) {
                e->coords = target->toWidgetCoordinates(e->coords);
                target->mouseUp(e);
            }
        }

        virtual void mouseClick(Event<MouseButtonEvent>::Payload & e, Widget * target) {
            if (target != nullptr && e.active()) {
                e->coords = target->toWidgetCoordinates(e->coords);
                target->mouseClick(e);
            }
        }

        virtual void mouseDoubleClick(Event<MouseButtonEvent>::Payload & e, Widget * target) {
            if (target != nullptr && e.active()) {
                e->coords = target->toWidgetCoordinates(e->coords);
                target->mouseDoubleClick(e);
            }
        }

        virtual void focusIn(Event<void>::Payload & e, Widget * target) {
            if (target != nullptr && e.active())
                target->focusIn(e);
        }

        virtual void focusOut(Event<void>::Payload & e, Widget * target) {
            if (target != nullptr && e.active())
                target->focusOut(e);
        }

        virtual void keyChar(Event<Char>::Payload & e, Widget * target) {
            if (target != nullptr && e.active())
                target->keyChar(e);
        }

        virtual void keyDown(Event<Key>::Payload & e, Widget * target) {
            if (target != nullptr && e.active())
                target->keyDown(e);
        }

        virtual void keyUp(Event<Key>::Payload & e, Widget * target) {
            if (target != nullptr && e.active())
                target->keyUp(e);
        }

        virtual void paste(Event<std::string>::Payload & e, Widget * target) {
            if (target != nullptr && e.active())
                target->paste(e);
        }
        //@}

        // ========================================================================================

        /** \name Child Widgets Management
         */
        //@{

        /** Called when a widget is attached to the renderer. 
         
            When attaching a subtree, the widgetAttached method will be called for each widget in the subtree. Called *after* the widget's renderer has been set. 
         */
        virtual void widgetAttached(Widget * widget) {
            UI_THREAD_CHECK;
            MARK_AS_UNUSED(widget);
            // TODO do nothing ? or make abstract?
        }

        /** Called when a widget is detached (removed from the tree).
         
            If a subtree is removed, the method is called for each widget starting at the leaves. At the call the widget is still attached. 
         */
        virtual void widgetDetached(Widget * widget) {
            UI_THREAD_CHECK;
            if (modalRoot_ == widget)
                modalRoot_ = rootWidget_;
            if (mouseFocus_ == widget) {
                Event<void>::Payload p{};
                widget->mouseOut(p);
                mouseFocus_ = nullptr;
            }
            if (keyboardFocus_ == widget) {
                NOT_IMPLEMENTED;
            }
            // make sure that if there were pending clipboard or selection requests for the widget, these are cancelled
            if (clipboardRequestTarget_ == widget)
                clipboardRequestTarget_ = nullptr;
            if (selectionRequestTarget_ == widget)
                selectionRequestTarget_ = nullptr;
        }
        //@}

        /** Returns the renderer's backing buffer. 
         */
        Buffer & buffer() {
            UI_THREAD_CHECK;
            return buffer_;
        }

        virtual void resize(int newWidth, int newHeight) {
            UI_THREAD_CHECK;
            if (buffer_.width() != newWidth || buffer_.height() != newHeight) {
                buffer_.resize(newWidth, newHeight);
                // resize the UI elements
                if (rootWidget_ != nullptr) {
                    rootWidget_->setRect(Rect::FromWH(newWidth, newHeight));
                    rootWidget_->visibleRect_ = Rect::FromWH(width(), height());
                    rootWidget_->bufferOffset_ = Point{0,0};
                    repaint(rootWidget_);
                }
            }
        }

    private:

        /** Queue of scheduled user events. */
        static std::deque<std::function<void(void)>> UserEvents_;
        /** UserEvents queue synchronization access */
        static std::mutex M_;

        static std::function<void(void)> UserEventScheduler_;

        Buffer buffer_;

        Color backgroundColor_;

        /** The root widget being rendered. */
        Widget * rootWidget_;
        /** The dominating element for the keyboard focus so that focusable elements can be limited to a given substree */
        Widget * modalRoot_;

        /** Determines if the mouse is captured by the window. Toggled by the mouseIn and mouseOut events.  */
        bool mouseIn_;
        /** The target for mouse events. */
        Widget * mouseFocus_;
        /** Number of mouse buttons pressed down. If greater than 0, then the mouse is captured by the mouse focus widget. */
        size_t mouseButtons_;

        /** Determines if the renderer's window is itself focused or not. */
        bool keyboardIn_;
        /** The target for keyboard events (focused widget). */
        Widget * keyboardFocus_;

        /** Widget which requested clipboard contents (if the focus changed since the request). */
        Widget * clipboardRequestTarget_;

        /** Widget which requested selection contents (if the focus changed since the request). */
        Widget * selectionRequestTarget_;

        /** Owner of the selection buffer, if the buffer is owned by a widget under the renderer. */
        Widget * selectionOwner_;

#ifndef NDEBUG
        /** \name UI Thread Checking. 
         */
        //@{
        friend class UiThreadChecker_;
        
        Renderer * getRenderer_() const {
            return const_cast<Renderer*>(this);
        }

        std::thread::id uiThreadId_;
        size_t uiThreadChecksDepth_ = 0;
        std::mutex uiThreadCheckMutex_;
        //@}
#endif

    }; // ui::Renderer

    /** Simplified renderer for local applications. 
     
        If the communication between the renderer and the UI elements is reliable (such as if they both run in the same process), the LocalRenderer offers a simplified API that handles things such as mouse clicks. 
     */
    class LocalRenderer : public Renderer {
    protected:
        /** \name Mouse Input 
         
            - rendererMouseOut
            - rendererMouseMove
            - rendererMouseWheel
            - rendererMouseDown
            - rendererMouseUp

         */
        //@{
        
        /** Triggered when mouse moves over the renderer. 
         
            If this is the first mouse event received after a mouse focus has been lost, the rendererMouseIn() method is called first to initiate the mouseIn event apporopriately. 
         */
        void rendererMouseMove(Point coords, Key modifiers) override {
            if (! mouseFocused())
                rendererMouseIn();
            Renderer::rendererMouseMove(coords, modifiers);
        }

        /** Triggered when mouse button is pressed. 
         
            If the button is the only button currently down, this can be the start of a mouse click so the current current time is stored in the mouse click time. Otherwise the click check state must be invalidated by setting mouseClickButton_ to 0. 
         */
        void rendererMouseDown(Point coords, MouseButton button, Key modifiers) override {
            Renderer::rendererMouseDown(coords, button, modifiers);
            // if this is the first button pressed, mark the time
            if (mouseButtonsDown() == static_cast<size_t>(button)) {
                mouseClickStart_ = helpers::SteadyClockMillis();
                mouseClickButton_ = mouseButtonsDown();
            // otherwise invalidate the click state
            } else {
                mouseClickButton_ = 0;
            }
        }

        /** Triggered when mouse button is released. 
         
            Triggers the mouseDown action and determines whether the button release consituted a click or a double click.
         */
        void rendererMouseUp(Point coords, MouseButton button, Key modifiers) override {
            if (mouseButtonsDown() != static_cast<size_t>(button) || mouseClickButton_ == 0) {
                Renderer::rendererMouseUp(coords, button, modifiers);
                mouseClickButton_ = 0;
                lastMouseClickWidget_ = 0;
            } else {
                // emit the mouse up
                rendererMouseUpEmit(coords, button, modifiers);
                ASSERT(mouseButtonsDown() == 0);
                // check if the mouse press time was short enough for a click
                size_t now = helpers::SteadyClockMillis();
                if (now - mouseClickStart_ <= MouseClickMaxDuration_) {
                    // if we have click, check whether the click is part of double click
                    if (
                        (lastMouseClickWidget_ == mouseFocus()) && // i.e. the same widget is focused
                        (lastMouseClickButton_ == static_cast<size_t>(button)) && // the same button
                        (mouseClickStart_ - lastMouseClickEnd_ <= MouseDoubleClickMaxDistance_) // fast enough
                    ) {
                        // emit the double click
                        rendererMouseDoubleClick(coords, button, modifiers);
                        // clear the double click state
                        lastMouseClickWidget_ = nullptr;
                    // it's not a double click, so emit the single click
                    } else {
                        rendererMouseClick(coords, button, modifiers);
                        // set the double click state in case the click becomes a double click in the future
                        lastMouseClickEnd_ = now;
                        lastMouseClickButton_ = static_cast<size_t>(button);
                        lastMouseClickWidget_ = mouseFocus();
                    }
                    // clear the mouse click state so that new clicks can be registered
                    mouseClickButton_ = 0;
                }
                // update the mouse target if necessary after the mouse capture has ended
                rendererMouseUpUpdateTarget(coords, modifiers);
            }
        }

        /** Changing mouse focus invalidates the possible double click state. 
         */
        void updateMouseFocus(Point coords) override {
            Renderer::updateMouseFocus(coords);
            if (lastMouseClickWidget_ != mouseFocus())
                lastMouseClickWidget_ = nullptr;
        }

        //@}

    protected:

        LocalRenderer(int width, int height):
            Renderer{width, height},
            mouseClickButton_{0},
            mouseClickStart_{0},
            lastMouseClickEnd_{0},
            lastMouseClickButton_{0},
            lastMouseClickWidget_{nullptr} {
        }
        
    private:

        /** Max number of milliseconds between a mouse click start and end. */
        static size_t MouseClickMaxDuration_;
        /** Max number of milliseconds between the end of first and start of second click within a double click. */
        static size_t MouseDoubleClickMaxDistance_;

        size_t mouseClickButton_;
        size_t mouseClickStart_;
        size_t lastMouseClickEnd_;
        size_t lastMouseClickButton_;
        Widget * lastMouseClickWidget_;

    }; // ui::LocalRenderer


} // namespace ui