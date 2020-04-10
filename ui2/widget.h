#pragma once

#include <mutex>

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

        virtual ~Widget() noexcept(false) {
            ASSERT(parent_ == nullptr) << "Widget must be detached from its parent before it is deleted";    
        }

        /** Determines if the given widget is transitive parent of the current widget or the widget itself. 
         */
        bool isDominatedBy(Widget const * w) const {
            UI_THREAD_CHECK;
            Widget const * x = this;
            while (x != nullptr) {
                if (x == w)
                    return true;
                x = x->parent_;
            }
            return false;
        }

        /** Returns the parent widget. 
         */
        Widget * parent() const {
            UI_THREAD_CHECK;
            return parent_;
        }


        /** Returns the rectangle occupied by the widget in its parent's contents area. 
         */
        Rect const & rect() const {
            return rect_;
        }

        int width() const {
            return rect_.width();
        }

        int height() const {
            return rect_.height();
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
        friend class Container;

        Widget(int width = 0, int height = 0, int x = 0, int y = 0):
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
        Event<void> onResize;
        Event<void> onMove;

        Event<void> onShow;
        Event<void> onHide;
        Event<void> onEnabled;
        Event<void> onDisabled;

        Event<void> onMouseIn;
        Event<void> onMouseOut;
        Event<MouseMoveEvent> onMouseMove;
        Event<MouseWheelEvent> onMouseWheel;
        Event<MouseButtonEvent> onMouseDown;
        Event<MouseButtonEvent> onMouseUp;
        Event<MouseButtonEvent> onMouseClick;
        Event<MouseButtonEvent> onMouseDoubleClick;

        Event<void> onFocusIn;
        Event<void> onFocusOut;
        Event<Char> onKeyChar;
        Event<Key> onKeyDown;
        Event<Key> onKeyUp;

        Event<std::string> onPaste;

        /** Sets the position of the cursor within given cavas. 
         
            Must be called in the paint() method and only currently focused widget can set cursor. 
         */
        void setCursor(Canvas & canvas, Cursor const & cursor, Point position);

        //@}

        /** \name Geometry

            - simple changes are ok
            - layouting is ok, but anchors are complicated, unless anchors are done via anchored layout, which will check all children and update those that have anchors specified, which is perhaps the way to go.   
            - YEAH !!!!

         */
        //@{

        /** Sets the position and size of the widget.
         
            Subclasses can override this method to inject modifications to the requested size and position. Doing so must be done with great care otherwise the automatic layouting can easily be broken. 
         */
        virtual void setRect(Rect const & value) {
            UI_THREAD_CHECK;
            // do nothing if the new rectangle is identical to the existing geometry
            if (rect_ == value)
                return;
            // update the rectangle
            bool wasResized = rect_.width() != value.width() || rect_.height() != value.height();
            bool wasMoved = rect_.topLeft() != value.topLeft();
            rect_ = value;
            // trigger the appropriate events
            if (wasResized)
                resized();
            if (wasMoved)
                moved();
            // inform the parent that the child has been resized, which should also trigger the parent's repaint and so ultimately repaint the widget as well
            if (parent_ != nullptr)
                parent_->childRectChanged(this);
        }

        /** Called *after* the widget has been resized. 
         
            Triggers the resize event. 
         */
        virtual void resized() {
            Event<void>::Payload p;
            onResize(p, this);
        }

        /** Called *after* the widget has been moved.
         
            Triggers the move event. 
         */
        virtual void moved() {
            Event<void>::Payload p;
            onMove(p, this);
        }

        /** Returns true if the widget is overlaid by other widgets. 
         
            Note that this is an implication, i.e. if the widget is not overlaid by other widgets, it can still return true. 
         */
        bool isOverlaid() const {
            UI_THREAD_CHECK;
            return overlaid_;
        }

        /** Changing the rectangle of a child widget triggers repaint of the parent. 
         */
        virtual void childRectChanged(Widget * child) {
            MARK_AS_UNUSED(child);
            repaint();
        }

        //@}

        // ========================================================================================

        /** \name Widget's Tree
         */
        //@{

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

        /** Returns the renderer of the widget. [thread-safe]
         */
        Renderer * renderer() const {
            return renderer_;
        }

        //@}

        // ========================================================================================

        /** \name Painting 
         */
        //@{

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

        //@}

        // ========================================================================================

        /** \name Mouse Input Handling
         
            Default implementation for mouse action simply calls the attached events when present as long as the propagation of the event has not been stopped. For more details about mouse events, see the \ref ui_renderer_mouse_input "mouse input" and \ref ui_renderer_event_triggers "event triggers" in ui2::Renderer.
         */
        //@{

        /** Triggered when the mouse enters the area of the widget. 
         */
        virtual void mouseIn(Event<void>::Payload & event) {
            if (event.active())
                onMouseIn(event, this);
        }

        /** Triggered when the mouse leaves the area of the widget. 
         */
        virtual void mouseOut(Event<void>::Payload & event) {
            if (event.active())
                onMouseOut(event, this);
        }

        /** Triggered when the mouse changes position inside the widget. 
         
            When mouse enters the widget, first mouseIn is called and then mouseMove as two separate events. When the mouse moves away from the widget then only mouseOut is called on the widget. 
         */
        virtual void mouseMove(Event<MouseMoveEvent>::Payload & event) {
            if (event.active())
                onMouseMove(event, this);
        }

        /** Triggered when the mouse wheel rotates while the mouse is over the widget, or captured. 
         */
        virtual void mouseWheel(Event<MouseWheelEvent>::Payload & event) {
            if (event.active())
                onMouseWheel(event, this);
        }

        /** Triggered when a mouse button is pressed down. 
         */
        virtual void mouseDown(Event<MouseButtonEvent>::Payload & event) {
            if (event.active())
                onMouseDown(event, this);
        }

        /** Triggered when a mouse button is released.
         */
        virtual void mouseUp(Event<MouseButtonEvent>::Payload & event) {
            if (event.active())
                onMouseUp(event, this);
        }

        /** Triggered when mouse button is clicked on the widget. 
         */
        virtual void mouseClick(Event<MouseButtonEvent>::Payload & event) {
            if (event.active())
                onMouseClick(event, this);
        }
        /** Triggered when mouse button is double-clicked on the widget. 
         */
        virtual void mouseDoubleClick(Event<MouseButtonEvent>::Payload & event) {
            if (event.active())
                onMouseDoubleClick(event, this);
        }

        /** Returns the mouse target within the widget itself corresponding to the given coordinates. 
         
            The default implementation returns the widget itself, but subclasses with child widgets must override this method and implement the logic to determine whether one of their children is the actual target. 
         */
        virtual Widget * getMouseTarget(Point coords) {
            MARK_AS_UNUSED(coords);
            return this;
        }

        /** Takes the renderer's coordinates and converts them to widget's coordinates.
         
            It is expected that this function is only called for visible widgets with valid positions, otherwise the functions asserts in debug mode and returns the origin otherwise. 
         */
        Point toWidgetCoordinates(Point rendererCoords) const {
            ASSERT(visible_ && renderer() != nullptr);
            // for robustness, return the origin if widget is in invalid state
            if (!visible_ || renderer() == nullptr) 
                return Point{0,0};
            return rendererCoords - bufferOffset_;
        }
        //@}

        // ========================================================================================

        /** \name Keyboard Input Handling
         
            Default implementation for keyboard actions simply calls the attached events when present.
         */
        //@{

        bool focused() const;

        virtual void focusIn(Event<void>::Payload & event) {
            if (event.active())
                onFocusIn(event, this);
        }

        virtual void focusOut(Event<void>::Payload & event) {
            if (event.active())
                onFocusOut(event, this);
        }

        virtual void keyChar(Event<Char>::Payload & event) {
            if (event.active())
                onKeyChar(event, this);
        }

        virtual void keyDown(Event<Key>::Payload & event) {
            if (event.active())
                onKeyDown(event, this);
        }

        virtual void keyUp(Event<Key>::Payload & event) {
            if (event.active())
                onKeyUp(event, this);
        }
        //@}

        // ========================================================================================

        /** \name Clipboard & Selection Actions

         */
        //@{
        /** Requests the clipboard contents to be returned via the paste() event. 
         */
        void requestClipboard();

        /** Requests the selection contents to be returned via the paste() event. 
         */
        void requestSelection();

        /** Paste event. 
         
            When a clipboard, or selection contents have been requested, the paste event is triggered when the contents is available. By default, the onPaste() event is triggered. 
         */
        virtual void paste(Event<std::string>::Payload & event) {
            if (event.active())
                onPaste(event, this);
        }

        /** Makes the current widget selection owner and informs the renderer of the ownership and selection contents. 
         */
        virtual void registerSelection(std::string const & contents);

        /** Gives up the selection and informs the renderer. 
         */
        virtual void clearSelection();

        /** Returns true if the widget currently holds the selection ownership. 
         */
        bool hasSelectionOwnership() const;

        //@}

    private:

        friend class Canvas;

        /** Mutex guarding the renderer property access. */
        mutable std::mutex mRenderer_;

        /** The renderer the widget is attached to. */
        Renderer* renderer_;

        /** Parent widget, nullptr if none. */
        Widget * parent_;

        /** The rectangle occupied by the widget in its parent's contents area. */
        Rect rect_;

        /** If true, parts of the widget can be overlaid by other widgets and therefore any repaint request of the widget is treated as a repaint request of its parent. */
        bool overlaid_;

        /** \anchor ui_widget_visible_rect 
            \name Visible Rectangle properties

            Each widget contains its visible rectangle and offset of its origin from the renderer's origin. These values can be used for immediate conversions of coordinates and painting on the renderer's canvas so that the canvas can easily determine the buffer coordinates and whether to render a particular cell or not. 
            
            Note that the values are only valid when the widget is visible and has a renderer attached. 

            \sa ui2::Canvas and its \ref ui_canvas_visible_rect "visible rectangle".
         */
        //@{
        /** The visible rectangle of the widget it its own coordinates. 
         */
        Rect visibleRect_;
        /** The top-left corner of the widget in the renderer's coordinates. 
         */
        Point bufferOffset_;
        //@}

        /** True if the widget should be visible. Widgets that are not visible will never get painted. 
         */
        bool visible_;

#ifndef NDEBUG
        friend class UiThreadChecker_;
        
        Renderer * getRenderer_() const  {
            return renderer_;
        }
#endif

    };

} // namespace ui2