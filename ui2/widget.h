#pragma once

#include <atomic>

#include "helpers/helpers.h"
#include "helpers/events.h"

#include "common.h"
#include "input.h"
#include "buffer.h"
#include "canvas.h"

namespace ui2 {

    class Canvas;
    class Renderer;

    class GeometryEvent {
    public:
        Rect rect;
        bool resized;
        bool moved;
    }; // GeometryEvent

    /** Base class for all UI widgets. 
     
        A widget can paint itself and can react to user interaction. 


        Setting the geometry must support the following scenarios:

        - manual

        The manual position is the default for widgets. It means that widget has control over its position 

        - anchors

        When parent resizes, the widget can update its own size as well based on the anchors, this will be done via the anchored trait. 

        => I need to notify the widget that parent has resized

        - layout managed

        => 



        # This deals with the manual settings:

        setRect() sets the size, can actually adjust the size, i.e. widgets can override the behavior and update the size before calling the parent's implementation

        The onRectChanged() event is then emitted, the event itself has flags for resize and reposition so that apps can tell 

        # Anchors 

        When parent gets resized, it informs all of its children and they are free to act on that knowledge. Or do nothing. When they do something, they can then resize themselves. 

        # Layout

        - when parent is resized, it needs to relayout the children, i.e. change their size and position according to its own layout

        - when child is resized, it needs to notify the parent so that the whole thing can be relaid out 

        - how to make these not cycle!!!!!


     */
    class Widget {
    public:

        virtual ~Widget() {
            // TODO how to deal with deleting attached widgets? 
        }

        /** Returns the rectangle occupied by the widget in its parent's contents area. 
         */
        Rect const & rect() const {
            return rect_;
        }

        /** Returns true if the widget is visible.
          
            Invisible widgets are not painted and do not receive any user input events (mouse or keyboard).  
         */
        bool visible() const {
            return visible_;
        }

        /** Triggers the repaint of the widget. [thread-safe]
         */
        virtual void repaint();

    protected:

        friend class Renderer;

        Widget(int x = 0, int y = 0, int width = 0, int height = 0):
            renderer_{nullptr},
            parent_{nullptr},
            rect_{Rect::FromTopLeftWH(x, y, width, height)},
            overlaid_{false},
            visible_{true} {
        }

        /** \name Events
         */
        //@{

        /** Triggered when the widget's geometry changes. 
         */
        Event<GeometryEvent> onGeometryChange;

        Event<void> onShow;
        Event<void> onHide;
        Event<void> onFocusIn;
        Event<void> onFocusOut;
        Event<void> onEnabled;
        Event<void> onDisabled;

        Event<MouseButtonEvent> onMouseDown;
        Event<MouseButtonEvent> onMouseUp;
        Event<MouseButtonEvent> onMouseClick;
        Event<MouseButtonEvent> onMouseDoubleClick;
        Event<MouseWheelEvent> onMouseWheel;
        Event<MouseMoveEvent> onMouseMove;
        Event<void> onMouseEnter;
        Event<void> onMouseLeave;

        Event<Char> onKeyChar;
        Event<Key> onKeyDown;
        Event<Key> onKeyUp;

        Event<std::string> onPaste;

        //@}

        /** \name Geometry

            - simple changes are ok
            - layouting is ok, but anchors are complicated, unless anchors are done via anchored layout, which will check all children and update those that have anchors specified, which is perhaps the way to go.   
            - YEAH !!!!
         */
        //@{

        /** Sets the position and size of the widget.
         
            Subclasses can override this method to inject modifications to the requested size and position. Doing so must be done with great care otherwise the automatic layouting can easily be broken. 

            TODO what to do with size hints? 

         */
        virtual void setRect(Rect const & value) {
            UI_THREAD_CHECK;
            // do nothing if the new rectangle is identical to the existing geometry
            if (rect_ == value)
                return;
            // update the rectangle
            bool resized = rect_.width() != value.width() || rect_.height() != value.height();
            bool moved = rect_.topLeft() != value.topLeft();
            rect_ = value;
            // raise the event
            Event<GeometryEvent>::Payload p{GeometryEvent{rect_, resized, moved}};
            onGeometryChange(p, this);
            // inform the parent that the child has been resized, which should also trigger the parent's repaint
            if (parent_ != nullptr)
                parent_->childRectChanged(this);
        }

        /** Changing the rectangle of a child widget triggers repaint of the parent. 
         */
        virtual void childRectChanged(Widget * child) {
            repaint();
        }

        /** Returns true if the widget is overlaid by other widgets. 
         
            Note that this is an implication, i.e. if the widget is not overlaid by other widgets, it can still return true. 
         */
        bool isOverlaid() const {
            UI_THREAD_CHECK;
            return overlaid_;
        }

        //@}

        /** Attaches the widget to given parent. 
         
            If the parent has valid renderer attaches the renderer as well.
         */
        virtual void attachTo(Widget * parent) {
            UI_THREAD_CHECK;
            ASSERT(parent_ == nullptr && parent != nullptr);
            parent_ = parent;
            if (parent_->renderer() != nullptr)
                attachRenderer(parent_->renderer());
        }

        /** Detaches the widget from its parent. 
         
            If the widget has renderer attached, detaches from the parent first and then detaches from the renderer.
         */
        virtual void detachFrom(Widget * parent) {
            UI_THREAD_CHECK;
            ASSERT(parent_ == parent);
            parent_ = nullptr;
            if (renderer() != nullptr)
                detachRenderer();
        }

        /** Attaches the widget to specified renderer. 
         
            The renderer must be valid and parent must already be attached if parent exists. 
         */
        virtual void attachRenderer(Renderer * renderer);

        /** Detaches the renderer. 
         
            If parent is valid, its renderer must be attached to enforce detachment of all children before the detachment of the parent. 
         */
        virtual void detachRenderer();

        /** Returns the parent widget. 
         */
        Widget * parent() const {
            UI_THREAD_CHECK;
            return parent_;
        }

        /** Returns the renderer of the widget. [thread-safe]
         */
        Renderer * renderer() const {
            return renderer_.load();
        }

        /** Returns the canvas to be used for drawing the contents of the widget. 
         */
        virtual Canvas getContentsCanvas(Canvas & canvas) {
            UI_THREAD_CHECK;
            return canvas;
        }

        /** Paints the widget on the given canvas. 
         
            The canvas is guaranteed to have the width and height of the widget itself. This method *must* be implemented in widget subclasses to actually draw the contents of the widget. 
         */
        virtual void paint(Canvas & canvas) = 0;

        /** \name Mouse Actions
         
            Default implementation for mouse action simply calls the attached events when present.
         */
        //@{
        void mouseDown(Event<MouseButtonEvent>::Payload & event) {
            onMouseDown(event, this);
        }
        void mouseUp(Event<MouseButtonEvent>::Payload & event) {
            onMouseUp(event, this);
        }
        void mouseClick(Event<MouseButtonEvent>::Payload & event) {
            onMouseClick(event, this);
        }
        void mouseDoubleClick(Event<MouseButtonEvent>::Payload & event) {
            onMouseDoubleClick(event, this);
        }
        void mouseWheel(Event<MouseWheelEvent>::Payload & event) {
            onMouseWheel(event, this);
        }
        void mouseMove(Event<MouseMoveEvent>::Payload & event) {
            onMouseMove(event, this);
        }
        void mouseEnter(Event<void>::Payload & event) {
            onMouseEnter(event, this);
        }
        void mouseLeave(Event<void>::Payload & event) {
            onMouseLeave(event, this);
        }
        //@}

        /** \name Keyboard Actions
         
            Default implementation for keyboard actions simply calls the attached events when present.
         */
        //@{
        void keyChar(Event<Char>::Payload & event) {
            onKeyChar(event, this);
        }
        void keyDown(Event<Key>::Payload & event) {
            onKeyDown(event, this);
        }
        void keyUp(Event<Key>::Payload & event) {
            onKeyUp(event, this);
        }
        //@}

        /** \name Clipboard Actions

            Default implementation the clipboard actions simply calls the attached events when present.
         */
        //@{
        void paste(Event<std::string>::Payload & event) {
            onPaste(event, this);
        }
        //@}

    private:

        friend class Canvas;

        /** The renderer the widget is attached to. */
        std::atomic<Renderer*> renderer_;

        /** Parent widget, nullptr if none. */
        Widget * parent_;

        /** The rectangle occupied by the widget in its parent's contents area. */
        Rect rect_;

        /** If true, parts of the widget can be overlaid by other widgets and therefore any repaint request of the widget is treated as a repaint request of its parent. */
        bool overlaid_;

        /** \name Visible rectangle properties

         */
        //@{
        Rect visibleRect_;
        Point bufferOffset_;
        //@}

        /** True if the widget should be visible. Widgets that are not visible will never get painted. 
         */
        bool visible_;

#ifndef NDEBUG
        friend class UiThreadChecker_;
        
        Renderer * getRenderer_() const  {
            return renderer_.load();
        }
#endif

    };

} // namespace ui2