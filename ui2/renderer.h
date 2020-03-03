#pragma once

#ifndef NDEBUG
#include <thread>
#include <mutex>
#endif

#include "common.h"
#include "buffer.h"
#include "widget.h"

namespace ui2 {


    /** UI Renderer
     
        Class responsible for rendering the widgets and providing the user actions such as keyboard, mouse and selection & clipboard.

        
     */
    class Renderer {
    public:

        std::string const & title() const {
            return title_;
        }

        void setTitle(std::string const & value) {
            UI_THREAD_CHECK;
            if (title_ != value) {
                title_ = value;
                titleChanged();
            }
        }

        /** Requests repaint of the given widget. [thread-safe]
         
            The purpose of this method is to use whatever event queue (or other mechanism) the target rendering supports to schedule repaint of the specified widget in the main UI thread. 
         */
        virtual void repaint(Widget * widget) = 0;

    protected:

        friend class Canvas;
        friend class Widget;

        /** \name Events
         
            - for keyboard best would be from 
         */
        //@{

        //@}

        /** Called when the selected rectangle of the backing buffer has been updated and needs rendered. 
         
            This method should actually render the contents. While only the refresh of the given rectangle is necessary at the time the method is called, the entire buffer is guaranteed to be valid in case whole screen repaint is to be issued. 
         */
        virtual void render(Rect const & rect) = 0;

        /** Called when the title of the renderer has changed. 
         */
        virtual void titleChanged() = 0;

        /** =======================================================================================
            \anchor ui_renderer_mouse_input
            \name Mouse Input

            The renderer is responsible for tracking the mouse pointer and report the mouse events to the affected widgets. Similarly to other user inputs, this is split into two layers. First the trigger functions in this section are invoked directly by the renderer subclasses that monitor the user inputs. Their task is to manage the mouse state and the mouse focus widget. 

            When these functions determine that particular mouse events should be triggered at the mouse focus widget, they call the appropriate \ref ui_renderer_event_triggers "event triggers" which then raise the event at the target widget. 

            All coordinates are relative to renderer's top-left corner. 
         */
        //@{
        /** Returns the widget that is current target for mouse events.
         
            This is either a widget over which the pointer is placed, or a widget that has captured the pointer. Note that the mouse focus can also be nullptr in case the mouse is not captured and outside of the renderer's window. 
         */
        Widget * mouseFocus() const {
            ASSERT(! mouseIn_ || mouseFocus_ != nullptr);
            return mouseFocus_;
        }

        virtual void rendererMouseIn() {
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
            // trigger the mouseOut event on existing mouse focus
            Event<void>::Payload p{};
            mouseOut(p, mouseFocus_);
            // clear mouse state
            mouseFocus_ = nullptr;
            mouseButtons_ = 0;
            mouseIn_ = false;
        }

        virtual void rendererMouseMove(Point coords, Key modifiers) {
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
         
            The mouseUp method is forwarded to the widget which has the mouse focus (mouseFocus())
         */
        virtual void rendererMouseUp(Point coords, MouseButton button, Key modifiers) {
            ASSERT(mouseIn_ && mouseButtons_ > 0);
            // this is technically not necessary as the mouse should be captured, but for added robustness the mouse focus is checked 
            updateMouseFocus(coords);
            mouseButtons_ &= ~ static_cast<size_t>(button);
            Event<MouseButtonEvent>::Payload p{MouseButtonEvent{coords, button, modifiers}};
            mouseUp(p, mouseFocus_);
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
        void updateMouseFocus(Point coords) {
            // don't change the mouse focus target if mouse is captured by a valid event
            if (mouseButtons_ != 0 && mouseFocus_ != nullptr)
                return;
            // determine the mouse target
            Widget * newTarget = getMouseTarget(coords);
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

        /** Determines the widget that should receive mouse events given the coordinates. 
         
            TODO this should act differently based on how the rendering is done, i.e. whether there are any modal widgets, etc. I need to figure out how these will work before proceeding. 

         */
        virtual Widget * getMouseTarget(Point coords) {
            MARK_AS_UNUSED(coords);
            NOT_IMPLEMENTED;
        }
        //@}

        /** =======================================================================================
            \anchor ui_renderer_event_triggers
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
        //@}



        /** \name Keyboard Input
         
            - who is going to keep track of modal windows and other things? 
         */
        //@{
        virtual void keyChar(Char c) {
            if (keyboardFocus_ == nullptr)
                return;
            Event<Char>::Payload payload{c};
            keyboardFocus_->keyChar(payload);
        }

        virtual void keyDown(Key k) {
            if (keyboardFocus_ == nullptr)
                return;
            Event<Key>::Payload payload{k};
            keyboardFocus_->keyDown(payload);
        }

        virtual void keyUp(Key k) {
            if (keyboardFocus_ == nullptr)
                return;
            Event<Key>::Payload payload{k};
            keyboardFocus_->keyUp(payload);
        }

        //@}

        /** \name Clipboard & Selection
         */
        //@{
        virtual void paste() {
            NOT_IMPLEMENTED;
        }
        //@}

        /** Called when a widget is attached to the renderer. 
         
            When attaching a subtree, the widgetAttached method will be called for each widget in the subtree. Called *after* the widget's renderer has been set. 
         */
        virtual void widgetAttached(Widget * widget) {
            UI_THREAD_CHECK;
            // TODO do nothing ? or make abstract?
        }

        /** Called when a widget is detached (removed from the tree).
         
            If a subtree is removed, the method is called for each widget starting at the leaves. At the call the widget is still attached. 
         */
        virtual void widgetDetached(Widget * widget) {
            UI_THREAD_CHECK;
            if (mouseFocus_ == widget) {
                Event<void>::Payload p{};
                widget->mouseOut(p);
                mouseFocus_ = nullptr;
            }
        }

        /** Returns the renderer's backing buffer. 
         */
        Buffer & buffer() {
            UI_THREAD_CHECK;
            return buffer_;
        }

        /** Immediately repaints the given widget. 
         
            If the widget is overlaid with another widgets, the widget parent will be painted instead. When the painting is done, i.e. the buffer has been updated, the render method is called to actually render the update.  
         */
        void paintWidget(Widget * widget);

    private:

        Buffer buffer_;

        /** Renderer's window title. 
         */
        std::string title_;

        /** The target for mouse events. 
         */
        Widget * mouseFocus_;
        /** Determines if the mouse is captured by the window. Toggled by the mouseIn and mouseOut events.  
         */
        bool mouseIn_;
        /** Number of mouse buttons pressed down. If greater than 0, then the mouse is captured by the mouse focus widget. 
         */
        size_t mouseButtons_;

        Widget * keyboardFocus_;


#ifndef NDEBUG
        friend class UiThreadChecker_;
        
        Renderer * getRenderer_() const {
            return const_cast<Renderer*>(this);
        }

        std::thread::id uiThreadId_;
        size_t uiThreadChecksDepth_;
        std::mutex uiThreadCheckMutex_;
#endif

    }; // ui::Renderer


} // namespace ui