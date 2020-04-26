#pragma once

#include <mutex>
#include <atomic>

#include "helpers/helpers.h"
#include "helpers/events.h"
#include "helpers/log.h"

#include "common.h"
#include "input.h"
#include "buffer.h"
#include "canvas.h"

namespace ui {

    class Canvas;
    class Renderer;
    class Layout;

    /** Defines a hint to the layout classes about widget dimensions. 
     */
    class SizeHint {
    public:
        enum class Kind : unsigned char {
            Percentage = 100,
            Layout,
            Manual,
            Auto,
        };

        SizeHint():
            value_{static_cast<unsigned char>(Kind::Layout)} {
        }

        /** Determines the percentage of available space the widget should occupy. 
         */
        static SizeHint Percentage(unsigned char pct) {
            ASSERT(pct < 101);
            return SizeHint{pct};
        }

        /** Leaves the size completely up to the contents of the widget. 
         
            This must be supported by the widget, if not supported, is identical to manual size hint. 
         */
        static SizeHint Auto() {
            return SizeHint{static_cast<unsigned char>(Kind::Auto)};
        }

        /** The size is determined by explicitly setting it.
         */
        static SizeHint Manual() {
            return SizeHint{static_cast<unsigned char>(Kind::Manual)};
        }

        Kind kind() const {
            if (value_ <= static_cast<unsigned char>(Kind::Percentage))
                return Kind::Percentage;
            return static_cast<Kind>(value_);
        }

        /** Calculates the size based on actual available and auto size values. 
         */
        int calculateSize(int currentSize, int layoutSize, int availableSize) {
            if (value_ <= static_cast<unsigned char>(Kind::Percentage))
                return availableSize * value_ / 100;
            else if (value_ == static_cast<unsigned char>(Kind::Layout))
                return layoutSize;
            else
                return currentSize;
        }

        bool operator == (SizeHint const & other) const {
            return value_ == other.value_;
        }

        bool operator != (SizeHint const & other) const {
            return value_ != other.value_;
        }

        bool operator == (Kind k) const {
            return kind() == k;
        }

        bool operator != (Kind k) const {
            return kind() != k;
        }

    private:

        SizeHint(unsigned char value):
            value_{value} {
        }

        unsigned char value_;

    }; // ui::SizeHint


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
#ifndef NDEBUG
            if (parent_ != nullptr) {
                LOG() << "Widget must be detached from its parent before it is deleted";
                std::terminate();
            }
#endif
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

        SizeHint widthHint() const {
            return widthHint_;
        }

        SizeHint heightHint() const {
            return heightHint_;
        }

        /** Returns true if the widget is visible.
          
            Invisible widgets are not painted and do not receive any user input events (mouse or keyboard).  
         */
        bool visible() const {
            return visible_;
        }

        bool enabled() const {
            return enabled_;
        }

        /** Triggers the repaint of the widget. [thread-safe]
         */
        virtual void repaint();

    protected:

        friend class Renderer;
        friend class Container;
        friend class Layout;
        template<template<typename> typename X, typename T>
        friend class TraitBase;

        Widget(int width = 0, int height = 0, int x = 0, int y = 0):
            pendingEvents_{0},
            renderer_{nullptr},
            repaintRequested_{false},
            parent_{nullptr},
            rect_{Rect::FromTopLeftWH(x, y, width, height)},
            overlaid_{false},
            paintDelegationRequired_{false},
            focusable_{false},
            visible_{true},
            enabled_{true} {
        }

        /** Schedules an user event to be executed in the main thread. [thread-safe]
         
            The scheduled event will be associated with the widget and if the widget is removed from its renderer before the event gets to be executed, the event is cancelled. 

            A widget can only send event if its renderer is attached. Events from unattached widgets are ignored. 
         */ 
        void sendEvent(std::function<void(void)> handler);

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

        //@}

        /** Sets the position of the cursor within given cavas. 
         
            Must be called in the paint() method and only currently focused widget can set cursor. 
         */
        void setCursor(Canvas & canvas, Cursor const & cursor, Point position);

        virtual void setVisible(bool value = true) {
            UI_THREAD_CHECK;
            if (visible_ != value) {
                visible_ = value;
                if (parent_ != nullptr)
                    parent_->childChanged(this);
            }
        }

        /** Determines whether the widget is enabled, or not. 
         
            If a focused widget is disabled, the widget looses focus passing it to the next focusable element in the renderer. 
         */
        virtual void setEnabled(bool value = true);

        /** \name Geometry

            - simple changes are ok
            - layouting is ok, but anchors are complicated, unless anchors are done via anchored layout, which will check all children and update those that have anchors specified, which is perhaps the way to go.   
            - YEAH !!!!

         */
        //@{

        virtual void setWidthHint(SizeHint value) {
            if (widthHint_ != value) {
                widthHint_ = value;
                if (widthHint_ == SizeHint::Kind::Auto)
                    autoSize();
                else if (parent_ != nullptr)
                    parent_->childChanged(this);
            }
        }

        virtual void setHeightHint(SizeHint value) {
            if (heightHint_ != value) {
                heightHint_ = value;
                if (heightHint_ == SizeHint::Kind::Auto)
                    autoSize();
                else if (parent_ != nullptr)
                    parent_->childChanged(this);
            }
        }

        /** Recalculates the size of the element based on its contents. 
         
            Only sets the dimensions whose size hint is set to SizeHint::Kind::Auto. To determine the sizes, the calculateAutoSize() method is called. To implement correct autosizing behavior, the calculateAutoSize() method should be overriden in the subclasses
         */
        void autoSize(bool forceRepaint = false) {
            if (widthHint_ != SizeHint::Kind::Auto && heightHint_ != SizeHint::Kind::Auto)
                return;
            std::pair<int, int> size = calculateAutoSize();
            Rect rect = Rect::FromTopLeftWH(
                rect_.topLeft(), 
                (widthHint_ == SizeHint::Kind::Auto) ? size.first : rect_.width(), 
                (heightHint_ == SizeHint::Kind::Auto) ? size.second : rect_.height()
                );
            // if the rectangle is the same, but repaint has been requested, call repaint
            if (rect == rect_) {
                if (forceRepaint)
                    repaint();
            // otherwise update the rectangle which would call repaint too
            } else {
                setRect(rect);
            }
        }

        /** For autosized elements, calculates the ideal width and height of the widget based on its contents. 
         
            Returns the pair of width and height. The default implementation returns the current width and height of the widget, but subclasses should reimplement this method to provide correct values based on the widget's contents. 

            It is the responsibility of the subclass widget to properly determine which dimension to calculate based on the widget's size hints, but to be on the safe side, the autosizing mechanism will only change dimensions whose size hints are set to SizeHint::Kind::Auto.  
         */
        virtual std::pair<int, int> calculateAutoSize() {
            return std::make_pair(rect_.width(), rect_.height());
        }

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
            // perform the autosizing if required by the widget
            // this has no effect if autosizing is not enabled, if autosizing is enabled it would trigger another resize
            autoSize();
            // if autosize changed the rectangle, it called setRect itself and updated the rectangle already so only emit the events and inform parent if the size has not changed
            if (rect_ == value) {
                // trigger the appropriate events
                if (wasResized)
                    resized();
                if (wasMoved)
                    moved();
                // inform the parent that the child has been resized, which should also trigger the parent's repaint and so ultimately repaint the widget as well
                if (parent_ != nullptr)
                    parent_->childChanged(this);
            }
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

        /** Changing a child widget triggers repaint of the parent. 
         */
        virtual void childChanged(Widget * child) {
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
         
            When the widget is to be repainted, its repaint() method must be called. This method can be called from any thread and its invocation triggers a repaint event in the renderer's UI thread, which then constructs the widget's canvas and calls its paint() method. 

            If a widget wishes to paint its children, it must call the paintChild() method with each appropriate child, giving it the child and the widget's contents canvas. 

            The contents should be determined by calling the getContentsCanvas(), which by default returns the widget's own canvas. Scrollable widgets, or widgets with borders should however override this method and construct an appropriate canvas on which the children will be painted. 
         
            Important part of the paint process is to determine when to delegate the paint request on a widget to its parent (which could be transitive). The shouldPaintParent() method determines for each widget whether its repaint should instead trigger repaint of its entire parent. The layout, the widget itself and its parents may all need the paint delegation for the reasons detailed below:

            The layout engine determines if the widget can be overlaid by one of its sibling widgets and if so, requests the paint method delegation so that the overlaid widget can be updated as well. This is set automatically by the layout object for all children of the widget in their overlaid_ fields. The overlaid_ value is not inherited from parent. 

            The widget itself may determine its repaint should always trigger its parent repaint in certain cases, such as when the widget is transparent. To do so, the method delegatePaintToParent() must be overriden in the subclass to detect such a situation. This value is not stored anywhere and the widget asked this on each repaint (and so is not inherited).

            Finally, the widget may require its children to delegate their repaints. This can be set on per child basis by overriding the requireChildToDelegatePaint() method and is useful for situations when the parent wishes to paint on the canvas *over* its children, such as when borders or scrollbars are used. 

            If a widget is overlaid, or if its parent required delegation, then the widget itself and all its children transitively must also delegate. This is stored in the paintDelegationRequired_ field which is inherited by children. 
         */
        //@{

        /** Returns true if the repainting the widget should be delegated to its parent instead. 
         */
        bool shouldPaintParent() {
            UI_THREAD_CHECK;
            return (overlaid_ || paintDelegationRequired_ || delegatePaintToParent()) && parent_ != nullptr;
        }

        /** Determines whether the widget itself has properties that require delegating its painting to the parent. 
         
            Such as if the widget is transparent, etc.
         */
        virtual bool delegatePaintToParent() {
            return false;
        }

        /** Returns true if the widget requires its children to delegate the paint. 
         
            This is useful if the widget has some rendering to perform *after* its children, such as borders or painted overlays.
         */ 
        virtual bool requireChildToDelegatePaint(Widget * child) {
            MARK_AS_UNUSED(child);
            return false;
        }

        /** Returns the canvas to be used for drawing the contents of the widget. 
         */
        virtual Canvas getContentsCanvas(Canvas & canvas) {
            UI_THREAD_CHECK;
            return canvas;
        }

        /** Paints the widget on the given canvas. 
         
            The canvas is guaranteed to have the width and height of the widget itself. This method *must* be implemented in widget subclasses to actually draw the contents of the widget. 

            The paint method should never be called directly by other widgets, which should use the paintChild() method instead. 
         */
        virtual void paint(Canvas & canvas) = 0;

        /** Paints given child widget. 
         
            Takes the widget itself and the contents canvas against which all children are positioned. 
         */
        void paintChild(Widget * child, Canvas & contentsCanvas) {
            ASSERT(child->parent() == this);
            // no need to paint children that are not visible
            if (! child->visible())
                return;
            // create the child's canvas
            Canvas childCanvas = contentsCanvas.clip(child->rect());
            // update the visible rect of the child according to the calculated canvas and repaint
            child->visibleRect_ = childCanvas.visibleRect();
            child->bufferOffset_ = childCanvas.bufferOffset_;
            // TODO this is not the most effective, the requireChildToDelegatePaint() can be precomputed if needs be
            child->paintDelegationRequired_ = overlaid_ || paintDelegationRequired_ || requireChildToDelegatePaint(child);
            child->paint(childCanvas);
            // once the child was painted, clear the repaint request flag
            child->repaintRequested_.store(false);
        }

        //@}

        // ========================================================================================

        /** \name Mouse Input Handling
         
            Default implementation for mouse action simply calls the attached events when present as long as the propagation of the event has not been stopped. For more details about mouse events, see the \ref ui_renderer_mouse_input "mouse input" and \ref ui_renderer_event_triggers "event triggers" in ui::Renderer.
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
            if (event.active()) {
                if (onMouseMove.attached()) {
                    event.propagateToParent(false);
                    onMouseMove(event, this);
                }
                if (event.shouldPropagateToParent() && event.active() && parent_ != nullptr) {
                    event->coords = parent_->toWidgetCoordinates(toRendererCoordinates(event->coords));
                    parent_->mouseMove(event);
                }
            }
        }

        /** Triggered when the mouse wheel rotates while the mouse is over the widget, or captured. 
         */
        virtual void mouseWheel(Event<MouseWheelEvent>::Payload & event) {
            if (event.active()) {
                if (onMouseWheel.attached()) {
                    event.propagateToParent(false);
                    onMouseWheel(event, this);
                }
                if (event.shouldPropagateToParent() && event.active() && parent_ != nullptr) {
                    event->coords = parent_->toWidgetCoordinates(toRendererCoordinates(event->coords));
                    parent_->mouseWheel(event);
                }
            }
        }

        /** Triggered when a mouse button is pressed down. 
         */
        virtual void mouseDown(Event<MouseButtonEvent>::Payload & event) {
            if (event.active()) {
                if (onMouseDown.attached()) {
                    event.propagateToParent(false);
                    onMouseDown(event, this);
                }
                if (event.shouldPropagateToParent() && event.active() && parent_ != nullptr) {
                    event->coords = parent_->toWidgetCoordinates(toRendererCoordinates(event->coords));
                    parent_->mouseDown(event);
                }
            }
        }

        /** Triggered when a mouse button is released.
         */
        virtual void mouseUp(Event<MouseButtonEvent>::Payload & event) {
            if (event.active()) {
                if (onMouseUp.attached()) {
                    event.propagateToParent(false);
                    onMouseUp(event, this);
                }
                if (event.shouldPropagateToParent() && event.active() && parent_ != nullptr) {
                    event->coords = parent_->toWidgetCoordinates(toRendererCoordinates(event->coords));
                    parent_->mouseUp(event);
                }
            }
        }

        /** Triggered when mouse button is clicked on the widget. 
         */
        virtual void mouseClick(Event<MouseButtonEvent>::Payload & event) {
            if (event.active()) {
                if (onMouseClick.attached()) {
                    event.propagateToParent(false);
                    onMouseClick(event, this);
                }
                if (event.shouldPropagateToParent() && event.active() && parent_ != nullptr) {
                    event->coords = parent_->toWidgetCoordinates(toRendererCoordinates(event->coords));
                    parent_->mouseClick(event);
                }
            }
        }
        /** Triggered when mouse button is double-clicked on the widget. 
         */
        virtual void mouseDoubleClick(Event<MouseButtonEvent>::Payload & event) {
            if (event.active()) {
                if (onMouseDoubleClick.attached()) {
                    event.propagateToParent(false);
                    onMouseDoubleClick(event, this);
                }
                if (event.shouldPropagateToParent() && event.active() && parent_ != nullptr) {
                    event->coords = parent_->toWidgetCoordinates(toRendererCoordinates(event->coords));
                    parent_->mouseDoubleClick(event);
                }
            }
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

            Note that this function can only be called from visible widgets with valid coordinates, otherwise the returned values are not guaranteed to be correct. 
         */
        Point toWidgetCoordinates(Point rendererCoords) const {
            ASSERT(visible_ && renderer() != nullptr);
            // for robustness, return the origin if widget is in invalid state
            if (!visible_ || renderer() == nullptr) 
                return Point{0,0};
            return rendererCoords - bufferOffset_;
        }

        /** Converts own coordinates to the global coordinates of the renderer. 

            Note that this function can only be called from visible widgets with valid coordinates, otherwise the returned values are not guaranteed to be correct. 
         */
        Point toRendererCoordinates(Point own) {
            ASSERT(visible_ && renderer() != nullptr);
            // for robustness, return the origin if widget is in invalid state
            if (!visible_ || renderer() == nullptr) 
                return Point{0,0};
            return own + bufferOffset_;
        }

        //@}

        // ========================================================================================

        /** \name Keyboard Input Handling
         
            Default implementation for keyboard actions simply calls the attached events when present.
         */
        //@{

        /** Returns whether the widget can receive keyboard focus. 
         */
        bool focusable() const {
            return focusable_;
        }

        /** Determines whether the widget can received keyboard focus. 
         */
        virtual void setFocusable(bool value = true) {
            if (focusable_ != value)
                focusable_ = value;
        }

        /** Returns the next focusable widget in the hierarchy. 

            If the currently focused widget is nullptr and the widget is focusable, returns itself, otherwise delegates to its parent, or returns nullptr if no parent exists. 
         */
        virtual Widget * getNextFocusableWidget(Widget * current) {
            ASSERT(current == nullptr || current == this);
            // if we are the current focusable widget delegate to parent if it exists, or return null
            if (current == this)
                return (parent_ == nullptr) ? nullptr : parent_->getNextFocusableWidget(current);
            // if current is null, and the widget can focused returns the widget
            if (current == nullptr && focusable_ && enabled_)
                return this;
            // otherwise returns nullptr
            return nullptr;
        }

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

        virtual void setClipboard(std::string const & contents);

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

        /** Number of pending user events registered with the current widget. [thread-safe]
         
            Only ever accessed via renderer which provides global synchronization via its user events lock. 
         */
        size_t pendingEvents_;

        /** Mutex guarding the renderer property access. */
        mutable std::mutex mRenderer_;

        /** The renderer the widget is attached to. */
        Renderer* renderer_;

        /** Determines if repaint of the widget has already been requested to limit multiple simultaneous repaint requests. 
         */
        std::atomic<bool> repaintRequested_;

        /** Parent widget, nullptr if none. */
        Widget * parent_;

        /** The rectangle occupied by the widget in its parent's contents area. */
        Rect rect_;

        /** Size hints for the layouters. */
        SizeHint widthHint_;
        SizeHint heightHint_;

        /** Determines if the widget is overlaid by its siblings. */
        bool overlaid_;

        /** If true, paint methods must be overlaid to the parent. Propagates to children. */
        bool paintDelegationRequired_;

        /** \anchor ui_widget_visible_rect 
            \name Visible Rectangle properties

            Each widget contains its visible rectangle and offset of its origin from the renderer's origin. These values can be used for immediate conversions of coordinates and painting on the renderer's canvas so that the canvas can easily determine the buffer coordinates and whether to render a particular cell or not. 
            
            Note that the values are only valid when the widget is visible and has a renderer attached. 

            \sa ui::Canvas and its \ref ui_canvas_visible_rect "visible rectangle".
         */
        //@{
        /** The visible rectangle of the widget it its own coordinates. 
         */
        Rect visibleRect_;
        /** The top-left corner of the widget in the renderer's coordinates. 
         */
        Point bufferOffset_;
        //@}

        /** True if the widget can receive keyboard focus. 
         */
        bool focusable_;

        /** True if the widget should be visible. Widgets that are not visible will never get painted. 
         */
        bool visible_;

        bool enabled_;
    }; // ui::Widget

    class PublicWidget : public Widget {
    public:
        using Widget::setVisible;
        using Widget::setWidthHint;
        using Widget::setHeightHint;

        using Widget::onMouseClick;

    protected:
        PublicWidget(int width = 0, int height = 0, int x = 0, int y = 0):
            Widget{width, height, x, y} {
        }

    }; // ui::PublicWidget

} // namespace ui