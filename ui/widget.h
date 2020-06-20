#pragma once

//#include <mutex>
#include <atomic>

#include "helpers/helpers.h"
#include "helpers/events.h"

#include "common.h"
#include "input.h"
#include "screen_buffer.h"
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

    /** Base class for all UI widgets. 
     
        A widget can paint itself and can react to user interaction. 

        ## Threads & Events

        All widget methods must be called from the main UI thread with the exception of:

        - repaint() method for requesting the repaint of the widget's contents

        - sendEvent() which can be used to schedule execution of arbitrary code in the main thread. 

        For more details on the events, see the Renderer class that is responsible for their actual scheduling and execution. 

        ## Painting

        To repaint the widget, the repaint() method can be called from any thread. The call schedules the main thread to analyze the layout & repaint information and ultimately trigger the render() method, which is responsible for the actual rendering of the widget. 
        
        The paint target analysis starts with the widget that wishes to painted and walks back all the way to the root element. On each widget, it checks whether a relayout is needed and offers the widget the chance to change the repaint target via the propagatePaintTarget() method. 

        The method has two arguments, sender, which is guaranteed to be the immediate child of the current widget, and target which is the widget currently scheduled to be repainted and returns the new paint target. Its default behavior deals with overlaid widgets, i.e. when the sender is overlaid, the target is replaced with itself. 
        
        To paint widget's children, the paintChild() method must be called with the child and its canvas. 

        - the #overlaid_ flag signifies that part of the widget might be obscured by other widgets, in which case the repaint method must be delegated to the parent so that the other widget will be updated as well. This property is set during the layout phase and propagates to children (i.e. an overlaid widget's children are all overlaid as well)

        ## Geometry & Layouts

        A widget position (rect()) and size (width(), height()) can be determined via the move() and resize() methods respectively. Width and height can further be refined by their respective size hints (SizeHint), which guide the layout calculations. 

        The relayout() method schedules repaint and relayouting to be executed before it. When a widget is relayouted, it should reclaculate the correct dimensions of its contents first and then of itself (if its size hints are set to SizeHint::Auto()). 

        WHen relayouting is requested, the calculateLayout() method is called immediately before the paint() method. Widget subclasses should override this method to implement layouting of their contents, but *must* call the Widget's implementation in the end as it performs the autosize calculation and clears the pending relayout flag. 

        In cases where the size hints are set to SizeHint::Auto() and the widget should therefore update its size according to the size of its contents, the relayout calls the calculateAutoSize() method which should return the ideal size of the widget and must be overriden in subclasses to provide the functionality. 

        Layout recalculation is triggered everytime children widgets change (move() or resize()).
     */
    class Widget {
    public:

        /** Destroys the widget and cancells all its pending events. 

            Note that destroying the widget while it (or someone else) can still raise events against it is undefined behavior and may end up with not all events cancelled. This is acceptable compromise as sending an event against the widget would mean having its valid pointer, which should not happen when the widget is being destroyed. 
         */
        virtual ~Widget();

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

        /** Returns the width of the widget. 
         */
        int width() const {
            return rect_.width();
        }

        /** Returns the height of the widget. 
         */
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

        /** Returns whether the widget is enabled. 
         
            Disabled widgets cannot receive keyboard focus.
         */
        bool enabled() const {
            return enabled_;
        }

        /** Repaints the widget [thread-safe]
         
            Calling this method requests a repaint of the entire widget and should be called whenever the information displayed in the widget changes. 

            The method can be called from any thread, but the repaint will always occur in the main UI thread. 
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
            pendingRepaint_{false},
            pendingRelayout_{false},
            overlaid_{false},
            parent_{nullptr},
            rect_{Rect::FromTopLeftWH(x, y, width, height)},
            focusable_{false},
            visible_{true},
            enabled_{true} {
        }

        /** \name Events
         */
        //@{

        /** Triggered when the widget's geometry changes. 
         */
        UIEvent<void> onResize;
        UIEvent<void> onMove;

        UIEvent<void> onShow;
        UIEvent<void> onHide;
        UIEvent<void> onEnabled;
        UIEvent<void> onDisabled;

        UIEvent<void> onMouseIn;
        UIEvent<void> onMouseOut;
        UIEvent<MouseMoveEvent> onMouseMove;
        UIEvent<MouseWheelEvent> onMouseWheel;
        UIEvent<MouseButtonEvent> onMouseDown;
        UIEvent<MouseButtonEvent> onMouseUp;
        UIEvent<MouseButtonEvent> onMouseClick;
        UIEvent<MouseButtonEvent> onMouseDoubleClick;

        UIEvent<void> onFocusIn;
        UIEvent<void> onFocusOut;
        UIEvent<Char> onKeyChar;
        UIEvent<Key> onKeyDown;
        UIEvent<Key> onKeyUp;

        UIEvent<std::string> onPaste;

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
            MARK_AS_UNUSED(parent);
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

        /** \name Mouse Input Handling
         
            Default implementation for mouse action simply calls the attached events when present as long as the propagation of the event has not been stopped. For more details about mouse events, see the \ref ui_renderer_mouse_input "mouse input" and \ref ui_renderer_event_triggers "event triggers" in ui::Renderer.
         */
        //@{

        /** Triggered when the mouse enters the area of the widget. 
         */
        virtual void mouseIn(UIEvent<void>::Payload & event) {
            if (event.active())
                onMouseIn(event, this);
        }

        /** Triggered when the mouse leaves the area of the widget. 
         */
        virtual void mouseOut(UIEvent<void>::Payload & event) {
            if (event.active()) 
                onMouseOut(event, this);
        }

        /** Triggered when the mouse changes position inside the widget. 
         
            When mouse enters the widget, first mouseIn is called and then mouseMove as two separate events. When the mouse moves away from the widget then only mouseOut is called on the widget. 
         */
        virtual void mouseMove(UIEvent<MouseMoveEvent>::Payload & event) {
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
        virtual void mouseWheel(UIEvent<MouseWheelEvent>::Payload & event) {
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
        virtual void mouseDown(UIEvent<MouseButtonEvent>::Payload & event) {
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
        virtual void mouseUp(UIEvent<MouseButtonEvent>::Payload & event) {
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
        virtual void mouseClick(UIEvent<MouseButtonEvent>::Payload & event) {
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
        virtual void mouseDoubleClick(UIEvent<MouseButtonEvent>::Payload & event) {
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

        /** Determines whether the widget can receive keyboard focus. 
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

        virtual void focusIn(UIEvent<void>::Payload & event) {
            if (event.active())
                onFocusIn(event, this);
        }

        virtual void focusOut(UIEvent<void>::Payload & event) {
            if (event.active())
                onFocusOut(event, this);
        }

        virtual void keyChar(UIEvent<Char>::Payload & event) {
            if (event.active()) {
                if (onKeyChar.attached()) {
                    event.propagateToParent(false);
                    onKeyChar(event, this);
                }
                if (event.shouldPropagateToParent() && event.active() && parent_ != nullptr)
                    parent_->keyChar(event);
            }
        }

        virtual void keyDown(UIEvent<Key>::Payload & event) {
            if (event.active()) {
                if (onKeyDown.attached()) {
                    event.propagateToParent(false);
                    onKeyDown(event, this);
                }
                if (event.shouldPropagateToParent() && event.active() && parent_ != nullptr)
                    parent_->keyDown(event);
            }
        }

        virtual void keyUp(UIEvent<Key>::Payload & event) {
            if (event.active()) {
                if (onKeyUp.attached()) {
                    event.propagateToParent(false);
                    onKeyUp(event, this);
                }
                if (event.shouldPropagateToParent() && event.active() && parent_ != nullptr)
                    parent_->keyUp(event);
            }
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
        virtual void paste(UIEvent<std::string>::Payload & event) {
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

        /** Schedules an user event to be executed in the main thread. [thread-safe]
         
            The scheduled event will be associated with the widget and if the widget is removed from its renderer before the event gets to be executed, the event is cancelled. 

            A widget can only send event if its renderer is attached. Events from unattached widgets are ignored. 
         */ 
        void sendEvent(std::function<void(void)> handler);

        virtual void setVisible(bool value = true) {
            UI_THREAD_CHECK;
            if (visible_ != value) {
                visible_ = value;
                if (parent_ != nullptr)
                    parent_->relayout();
                else
                    repaint();
            }
        }

        /** Determines whether the widget is enabled, or not. 
         
            If a focused widget is disabled, the widget looses focus passing it to the next focusable element in the renderer. 
         */
        virtual void setEnabled(bool value = true);

        virtual void setWidthHint(SizeHint value) {
            if (widthHint_ != value) {
                widthHint_ = value;
                if (widthHint_ == SizeHint::Kind::Auto)
                    relayout(); 
            }
        }

        virtual void setHeightHint(SizeHint value) {
            if (heightHint_ != value) {
                heightHint_ = value;
                if (heightHint_ == SizeHint::Kind::Auto)
                    relayout();
            }
        }

        /** Allows the widget to update the paint target during the paint event propagation. 
         
            The sender is the immediate child from whose subtree the paint event originates and the target is the widget that is currently being repainted. 

            The method returns the new paint target widget, which by default is the actual target, unless the sender is overlaid, in which case the new target widget is the current widget itself so that the widget's overlay can be dealt with. 

            The idea of this method is to give parent widgets the opportunity to widen the paint area when necessary. 
         */
        virtual Widget * propagatePaintTarget(Widget * sender, Widget * target) {
            return sender->overlaid_ ? this : target;
        }

        /** Renders the widget immediately. 
         */
        virtual void render();


        /** Requests the relayout of the widget. 
         */
        void relayout() {
            UI_THREAD_CHECK;
            if (! pendingRelayout_) {
                pendingRelayout_ = true;
                repaint();
            }
        }

        /** Paints given child on the specified canvas. 
         
            Only visible children are painted. Before repainting the child, updates its visible rectangle, buffer offset and determines whether the child should delegate its repainting to the parent (see requireRepaintParentFor() for more details). 

            Clears the #pendingRepaint_ flag before repainting the child as well. 
         */
        void paintChild(Widget * child, Canvas & childCanvas) {
            UI_THREAD_CHECK;
            ASSERT(child->parent() == this);
            ASSERT(child->width() == childCanvas.width() && child->height() == childCanvas.height());
            // don't do anything if the child is not visible
            if (! child->visible())
                return;
            // update the visible rect of the child according to the calculated canvas and repaint
            child->visibleRect_ = childCanvas.visibleRect();
            child->bufferOffset_ = childCanvas.bufferOffset_;
            // clear the child repaint flag as long as its relayout is not requested (if its relayout is requested, there is another pending repaint and it is not us)
            if (! child->pendingRelayout_)
                child->pendingRepaint_.store(false);
            // repaint the child actually
            child->paint(childCanvas);
        }

        /** Sets the position of the cursor within given cavas. 
         
            Must be called in the paint() method and only currently focused widget can set cursor. 
         */
        void setCursor(Canvas & canvas, Cursor const & cursor, Point position);

        /** Draws the widget's contents on the canvas. 
         
            Must be implemented by the widget subclasses. When paint() method is called, the dimensions of the widget and all its children is guaranteed to be correct and only the painting should occur. 
         */
        virtual void paint(Canvas & canvas) {
            UI_THREAD_CHECK;
            ASSERT(canvas.width() == width() && canvas.height() == height());
            MARK_AS_UNUSED(canvas);
        }

        /** Calculates the layout of the widget and any of its children. 
         
            For widget, simply determines the autosize if required. 

            TODO container first relayouts its children via the attached layout and then calls the parent implementation.
         */
        virtual void calculateLayout() {
            UI_THREAD_CHECK;
            if (widthHint_ == SizeHint::Kind::Auto || heightHint_ == SizeHint::Kind::Auto) {
                Size size(calculateAutoSize());
                if (widthHint_ != SizeHint::Kind::Auto)
                    size.setWidth(width());
                if (heightHint_ != SizeHint::Kind::Auto)
                    size.setHeight(height());
                // resize itself
                resize(size.width(), size.height());
            }
            pendingRelayout_ = false;
        }

        /** Calculates the automatic size of the widget. 
         
            Both width and height must be returned, but only dimensions whose size hints are set to Auto will be used. That said, correct autosizing algorithms should only return different values in those dimensions for which autosizing is enabled (an example is a label widget, which if both dimensions are autosized will have the width of its longest line and height of the number of lines, but if only height is autosized, its width will be unchanged and height will be the number of rows required to display the whole text given the width).
         */
        virtual Size calculateAutoSize() {
            UI_THREAD_CHECK;
            return Size{width(), height()};
        }

        /** Moves the widget to different location without changing its size. 
         
            It is not necessary to relayout the widget itself during the move as its size stays the same. Moving a widget triggers a relayout of its parent, which may also move the widget, perhaps even to its original position. However since the relayout is scheduled, a call to the move() method actually always moves the widget (as long as the coordinates are different from the existing ones). Since parent's relayout will eventually repaint the widget, a move will only be visible after it is corrected by the parent's relayout (if at all). 
         */
        virtual void move(Point topLeft) {
            UI_THREAD_CHECK;
            // don't do anything if noop
            if (topLeft == rect_.topLeft())
                return;
            // move the widget
            rect_ = Rect::FromTopLeftWH(topLeft, rect_.width(), rect_.height());
            // if we have parent, inform parent it needs to relayout which would then trigger repaint. If the parent's relayout is already scheduled, or in progress, the move is caused by it and nothing more needs to be done
            if (parent_ != nullptr) {
                // parent's relayout would repaint us too, so no need to trigger more 
                pendingRepaint_ = true;
                parent_->relayout();
            }
            // trigger the onMove event
            UIEvent<void>::Payload p;
            onMove(p, this);
        }

        /** Resizes the widget. 
         
            Note that resizing a widget does not guarantee that after the call the widget will have the requested width and height as resizing triggers relayouting immediately, which may in turn autosize the widget, in which case the size of the widget afterards will be the calculated autosize. 

            For this reason any layout and autosize calculation must eventually converge, otherwise the resize and relayout might get stuck in an infinite recursion. Calling resize with new size is guaranteed to generate precisely one onResize event, however it is possible that the widget will not in fact be resized - if widget is resized, but then the autosize is calculated to its existing size, so no size change would occur, but onResize event will be triggered. 
         */
        virtual void resize(int width, int height) {
            UI_THREAD_CHECK;
            // don't do anything if the width and height are identical
            if (rect_.width() == width && rect_.height() == height)
                return;
            // resize the widget
            rect_ = Rect::FromTopLeftWH(rect_.topLeft(), width, height);
            // the size of the widget has changed, we need to relayout, since we are already in the UI thread, relayout can happen immediately
            pendingRelayout_ = true;
            calculateLayout();
            // relayouting could have triggered another resize(s) in case of autosize calculation after the layout, so firsty check if the current resize is stil valid
            if (rect_.width() != width || rect_.height() != height)
                return;
            // now we need to inform the parent that it needs to relayout. If the parent is already in the middle of the layout, then this will be muted, which is ok, as the parent initiated the resize itself. Otherwise the parent's relayout will eventually repaint the current widget as well
            if (parent_ != nullptr) {
                // parent's relayout would repaint us too, so no need to trigger more 
                pendingRepaint_ = true;
                parent_->relayout();
            // if there is no parent then we simply need to repaint since the layout has already been calculated
            } else {
                repaint();
            }
            // finally, trigger the onResize event
            UIEvent<void>::Payload p;
            onResize(p, this);
        }

    private:

        friend class Canvas;

        /** Number of pending user events registered with the current widget. [thread-safe]
         
            Only ever accessed via renderer which provides global synchronization via its user events lock. 
         */
        size_t pendingEvents_;

        /** Mutex guarding the renderer property access. */
        //mutable std::mutex mRenderer_;

        /** The renderer the widget is attached to. */
        Renderer* renderer_;

        /** Indicates that a repaint has been requested and there is no need to request it again as the painting has not happened yet. 
         */
        std::atomic<bool> pendingRepaint_;

        /** Indicates that relayout has been requested. Whenever relayout is requested, repaint is requested as well and the relayout happens just before the repaint. 
         */
        bool pendingRelayout_;

        /** Determines whether parts of the widget can be covered by its siblings (layout-dependent). 
         */
        bool overlaid_;

        /** Parent widget, nullptr if none. */
        Widget * parent_;

        /** The rectangle occupied by the widget in its parent's contents area. */
        Rect rect_;

        /** Size hints for the layouters. */
        SizeHint widthHint_;
        SizeHint heightHint_;

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
        using Widget::setEnabled;
        using Widget::setWidthHint;
        using Widget::setHeightHint;

        using Widget::onMouseClick;

    protected:
        PublicWidget(int width = 0, int height = 0, int x = 0, int y = 0):
            Widget{width, height, x, y} {
        }

    }; // ui::PublicWidget

} // namespace ui