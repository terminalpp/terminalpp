#pragma once

#include <thread>
#include <atomic>
#include <deque>

#include "helpers/helpers.h"

#include "events.h"
#include "canvas.h"
#include "layout.h"

#ifndef NDEBUG

#define UI_THREAD_ONLY if (Widget::UIThreadID() != std::this_thread::get_id()) THROW(Exception()) << "Only UI thread is allowed to execute at this point"
#else

#define UI_THREAD_ONLY
#endif


namespace ui {

    class Widget;

    /** Base class for all ui widgets. 
     
        
     
     
     
     */
    class Widget {
        friend class Renderer;
        friend class Layout;
        friend class EventQueue;
        friend class Dismissable;
        friend class SizeHint::AutoSize;
    public:

        Widget() = default;
        
        virtual ~Widget() {
            for (Widget * child : children_)
                delete child;
            // hints are owned
            delete widthHint_;
            delete heightHint_;
            // layout is owned
            delete layout_;
        }

    // ============================================================================================
    /** \name Event Scheduling
     
        The widget has a shorthand schedule() method which can be used to schedule a new event linked to the widget. 

        Each widget also remembers the number of pending events, i.e. events that were scheduled, but not yet executed so that when the widget is detached, the events can be cancelled automatically. 
     */
    //@{
#ifndef NDEBUG 
    public: 
        /** Check to make sure that code is executed in main UI thread. 
         
            This is only enabled in debug builds. See the UI_THREAD_ONLY macro for more details. 
         */
        static std::thread::id UIThreadID() {
            static std::thread::id id{std::this_thread::get_id()};
            return id;
        }
#endif

    protected:
        /** Schedules given event and links it to the current widget. 
         
            Does nothing if the widget is not attached to a renderer. 
         */
        void schedule(std::function<void()> event);

    private:
        /** Number of pending events linked to the widget. 
         
            These are only accessed from the event queue and such are protected by the event queue access mutex. 
         */
        size_t pendingEvents_{0};

    //@}

    // ============================================================================================

    /** \name Widget Tree
     
        The widgets are arranged in a tree. 
        */
    //@{

    public:

        /** Returns the parent widget. 
         
            If the widget has no parent (is unattached, or is a root widget), returns false. 
        */
        Widget * parent() const {
            return parent_;
        }

        /** Returns true if the widget dominates the current one in the widget tree. 

            Widget is dominated by itself and by its own parents transitively. The root widget dominates *all* widgets. 
            */
        bool isDominatedBy(Widget const * widget) const;

        /** Returns the closest common parent of itself and the widget in argument.

            In graph theory, this is the Lowest Common Ancestor.  
            */
        Widget * commonParentWith(Widget const * other) const;

        /** Given renderer (window) coordinates, returns those coordinates relative to the widget. 
         
            Can only be called for widgets which are attached to a renderer, translates the coordinates irrespectively of whether they belong to the target widget, or not. 
            */
        Point toWidgetCoordinates(Point rendererCoords) const {
            ASSERT(renderer_ != nullptr);
            return rendererCoords - visibleArea_.offset();
        }

        /** Given widget coordinates, returns those coordinates relative to the renderer's area (the window). 
         
            Can only be called for widgets which are attached to a renderer, translates the coordinates irrespectively of whether they are visible in the window, or not. 
            */
        Point toRendererCoordinates(Point widgetCoords) const {
            ASSERT(renderer_ != nullptr);
            return widgetCoords + visibleArea_.offset();
        }

        /** Converts the widget coordinates to the coordinates of the widget contents. 
         */
        Point toContentsCoords(Point widgetCoords) {
            return widgetCoords + scrollOffset();
        }

        /** Returns the widget that is directly under the given coordinates, or itself. 
         */
        Widget * getMouseTarget(Point coords) {
            for (Widget * child : children_) 
                if (child->rect_.contains(coords))
                    return child->getMouseTarget(child->toWidgetCoordinates(toRendererCoordinates(coords)));
            return this;
        }

        Widget * nextWidget(std::function<bool(Widget*)> cond = nullptr) {
            return nextWidget(cond, this, true);
        }

        Widget * prevWidget(std::function<bool(Widget*)> cond = nullptr) {
            return prevWidget(cond, this, true);
        }
        
    protected:

        /** Adds given widget as a child so that it will be in front (painted after all its siblings).
         * 
         */
        virtual void attach(Widget * child);

        /** Adds given widget as so that it will be in the back (painted before all its siblings so that they can paint over it).
         */
        virtual void attachBack(Widget * child);

        /** Removes given widget from the child widgets. 
         */
        virtual void detach(Widget * child);

        /** Returns true if the widget is a root widget, i.e. if it has no parent *and* is attached to a renderer. 
         */
        bool isRootWidget() const {
            return (parent_ == nullptr) && (renderer_ != nullptr);
        }

        std::deque<Widget *> const & children() const {
            return children_;
        }

        Widget * nextWidget(std::function<bool(Widget*)> cond, Widget * last, bool checkParent);

        Widget * prevWidget(std::function<bool(Widget*)> cond, Widget * last, bool checkParent);

    private:
        /** Mutex protecting the renderer pointer for the widget. 
         */
        std::mutex rendererGuard_;

        Widget * parent_ = nullptr;
        std::deque<Widget *> children_;

    //@}


    // THIS IS THE NEW LAYOUTING AND PAINTING DRAFT

    /** \name Layouting and Painting 
     
        WRONG:

        Although two things, the painting and layouting are intertwined. Both can be requested from any thread (requestRepaint() and requestRelayout()). Outstanding requests for both are flagged so that the actual repaint and relayouts are scheduled only once and subsequent requests before the actual action takes place are ignored. 

        ### Painting 

        Repaint request is ignored if there is outstanding repaint, or relayout request. The repaint() method is then scheduled in the UI thread. This method verifies that the repaint is still valid (i.e. no relayout has been scheduled since the request was made) and then verifies that none of the parents of the widget invalidates the request (such as when the parent has pending relayout), or steals the request for itself (when it has pending repaint) via the allowRepaintRequest() method.

        If all the checks are ok, the renderer's repaint method is called, which in turn triggers the paint() method in the widget. This indirection over the renderer allows the renderer to have final say over when & what to actually repaint (i.e. when the renderer does not do immediate repaints, but uses fps). 

        When the renderer actually wishes to repaint the widget, it calls the paint() method, which creates the canvas for the widget, paints the background, calls the paint(Canvas &) method that should be overriden to actually paint the widget on given canvas and finally draws the widget's border if any (borders have to be drawn last). 

        ### Visible Rectangles



        ### Layouting 

        A widget's layouting is responsible for two things: (1) determining the layout (position and size) of the widget and its children and (2) keeping the visible rectangles in sync with the actual position and size. Since the relayout request is usually expected to happen after a UI property changes, which must happen in the main thread, the layouting does not provide the requesting capacity and when relayout() method is called, the relayouting proceeds immediately. 

        




     */
    //@{

    public:

        virtual void move(Point newTopLeft) {
            UI_THREAD_ONLY;
            if (rect_.topLeft() == newTopLeft)
                return;
            rect_.move(newTopLeft);
            // inform the parent to relayout, which will also update our visible area. If there is no parent, then we are not attached and there is no need to update the area as it will be updated when we are attached
            if (parent_ != nullptr)
                parent_->relayout();
            // trigger onMove if our topLeft is still the newTopLeft (i.e. the relayout did not change the position, if it did, then the onMove has already been triggered)
            //NOT_IMPLEMENTED;
        }

        virtual void resize(Size newSize) {
            UI_THREAD_ONLY;
            if (rect_.size() == newSize) 
                return;
            rect_.resize(newSize);
            // if we can relayout the parent, relayout it as its relayout will relayout us as well and since our size changed, the parent might have to adjust others.
            // if we have no parent, we are root widget, or detached and must relayout ourselves
            // if we have parent and it is relayouting currently, then the resize comes from its relayout and we have to relayout ourselves as part of it
            if (parent_ == nullptr || ! parent_->relayout())
                relayout();
            // finally trigger the on resize event in case the above relayouts did not change the size themselves
            //NOT_IMPLEMENTED;
        }

    protected:
        /** Actually paints the contents of the widget. 

            Override this method in subclasses to actually paint the widget's contents using the provided canvas. The default implementation simply paints the widget's children. 

            This function is called automatically by the repainting mechanics and should not be called by the class itself. To explicitly repaint a widget, use the scheduleRepaint() method.  
         */
        virtual void paint(Canvas & canvas) {
            UI_THREAD_ONLY;
            MARK_AS_UNUSED(canvas);
            for (Widget * child : children_) {
                if (child->visible())
                    child->paint();
            }
        }

        /** Schedules widget's repaint in the UI thread.
         
            The repaint will not be scheduled if there is already a repaint requested on the widget, or if there is pending relayout request, as the relayout will trigger repaint when finished. 
         */
        void requestRepaint() {
            unsigned expected = NONE;
            if (! requests_.compare_exchange_strong(expected, REQUEST_REPAINT, std::memory_order_acq_rel))
                return;
            // schedule the repaint
            schedule([this](){
                repaint();
            });
        }

        /** Relayouts the widget's contents (possibly changing own size). 
         
            Returns true if the relayout happened (even if nothing changed) and false if the relayout was skipped (i.e. because it is already happening, which could happen when relayouted widget calls resize or move on its child, which in turn triggers its parent's relayout).

            Relayouting a widget repositions and resizes all child widgets first. Once the children have correct sizes and positions, the current widget may still be resized if any of its dimensions is autosized. If this changes the size of the widget, then the relayout of its parent, or if relayouting, the widget itself is triggered. This could in theory mean a cycle, so to break the cycle, a count of relayout autosize depth is calculated and if the sizes won't converge, the layouting is stopped.  
            
         */
        virtual bool relayout() {
            UI_THREAD_ONLY;
            if (requests_.fetch_or(RELAYOUTING) & RELAYOUTING)
                return false;
            // layout the children widgets
            layout_->layout(this);
            // calculate contentsRect
            contentsRect_ = Rect{};
            for (Widget * child : children_)
                if (child->visible())
                    contentsRect_ = contentsRect_ | child->rect();
            // after the position and size of child widgets is calculated, we have to check if the widget itself should be resized in case its size hints are autosized, after which the layouting can finish in two different ways:
            Size size = getAutoSizeHint();
            if (relayoutDepth_ <= 2 && rect_.size() != size) {
                // if the autosized size differs from current size, end the layouting prematurely and resize itself. This would trigger relayout of the parent, or if the parent is relayouting, own relayout
                requests_.fetch_and(~RELAYOUTING);
                // increase relayout depth and resize the widget, which will eventually trigger its relayout again
                ++ relayoutDepth_;
                resize(size);
            } else {
                // reset the relayout depth as we are finished now
                relayoutDepth_ = 0;
                // own size and layout are valid, we are done relayouting, calculate overlays
                layout_->calculateOverlay(this);
                // if the widget is the root of relayouting (no parent, or parent is not relayouting), update the visible area of the widget and its children
                if (parent_ == nullptr || ! (parent_->requests_.load() & RELAYOUTING)) {
                    updateVisibleArea();
                    requestRepaint();
                }
                // clear the flag and return success
                requests_.fetch_and(~RELAYOUTING);
            }
            return true;
        }

        /** Calculates the size of the widget taking auto size hints (if any) into account. 
         
            The width and height, if autosized as per the respective hints is calculated indirectly through the hint so that any hint specified constraints can be fulfilled. 

            Returns always the current dimension for those where the hint is not set to autosize.
         */
        Size getAutoSizeHint() const {
            Size result = rect_.size();
            if (widthHint_->isAuto())
                result.setWidth(widthHint_->calculateWidth(this, 0, 0));
            if (heightHint_->isAuto())
                result.setHeight(heightHint_->calculateHeight(this, 0, 0));
            return result;
        }

    private:

        /** Determines whether repaint request of given immediate child should be allowed or not. 
         
         */
        virtual bool allowRepaintRequest(Widget * immediateChild) {
            UI_THREAD_ONLY;
            // if the parent has its own pending repaint, deny the child repaint as it will be repainted as a result of it
            if (requests_.load() & REQUEST_REPAINT)
                return false;
            // if the child from which the request comes is overlaid, then block the request and repaint itself instead
            // the same if there is border on this widget
            // TODO the border only needs top be repainted if the widget touches it
            if (immediateChild->overlaid_ || ! border_.empty() || ! immediateChild->background_.opaque()) {
                repaint();
                return false;
            }
            // otherwise defer to own parent, or allow if root element
            if (parent_ != nullptr)
                return parent_->allowRepaintRequest(this);
            else
                return true;
        }

        /** Implements the part of repaint request that executes in the UI thread. 
         */
        void repaint();

        /** Immediately paints the widget. 
         
            This method is to be used when another widget is to be painted as part of its parent. It clears the pending repaint flag, unlocking future repaints of the widgets, creates the appropriate canvas and calls the paint(Canvas &) method to actually draw the widget. 

            This function is called automatically by the repainting mechanics and should not be called by the class itself. To explicitly repaint a widget, use the scheduleRepaint() method.  
         */        
        void paint();

        static constexpr unsigned NONE = 0;
        static constexpr unsigned REQUEST_REPAINT = 1;
        static constexpr unsigned RELAYOUTING = 2;
        std::atomic<unsigned> requests_ = 0;
        unsigned relayoutDepth_ = 0;

    //@}



    // ============================================================================================

    /** \name Layouting
     */

    //@{

    public:

        /** Returns whether the widget is visible or not. 

            Visible widget does not guarantee that the widget is actually visible for the end user, but merely means that the widget should be rendered when appropriate. Invisible widgets are never rendered and do not occupy any layout space.  
        */
        bool visible() const {
            return visible_;
        }

        virtual void setVisible(bool value = true) {
            if (visible_ != value) {
                visible_ = value;
                if (parent_ != nullptr)
                    parent_->relayout();
            }
        }

        /** Returns the rectangle the widget occupies in its parent's contents area. 
         */
        Rect const & rect() const {
            return rect_;
        }

        Size size() const {
            return rect_.size();
        }

        int width() const {
            return rect_.width();
        }

        int height() const {
            return rect_.height();
        }

        SizeHint const * widthHint() const {
            return widthHint_;
        }

        virtual void setWidthHint(SizeHint * value) {
            if (widthHint_ != value) {
                delete widthHint_;
                widthHint_ = value;
                if (parent_ != nullptr)
                    parent_->relayout();
            }
        }

        SizeHint const * heightHint() const {
            return heightHint_;
        }

        virtual void setHeightHint(SizeHint * value) {
            if (heightHint_ != value) {
                delete heightHint_;
                heightHint_ = value;
                if (parent_ != nullptr)
                    parent_->relayout();
            }
        }

        /** Returns the contents rectangle.
         
            The rectangle is in client coordinates and encompasses all of the widget's children fully. It is useful to determine if scrolling is necessary or not (i.e. if clientSize < contentsRect.size())
         */
        Rect contentsRect() const {
            return contentsRect_;
        }

        /** Returns the size of the content's canvas. 
         
            Contents canvas is at least as big as the parent widget itself union with the positive part of the contentsRect(). 
         */
        Size contentsCanvasSize() const {
            Rect crect = contentsRect();
            return Size{
                std::max(width(), crect.right()),
                std::max(height(), crect.bottom())
            };
        }

    protected:

        Layout * layout() const {
            return layout_;
        }

        virtual void setLayout(Layout * value) {
            if (layout_ != value) {
                delete layout_;
                layout_ = value;
                relayout();
            }
        }

        /** Returns the canvas used for the contents of the widget. 

            The canvas has the size of the contentsRect()'s positive part or the widget's size, whatever is greater and is scrolled according to the widget's scrollOffset()
         */
        Canvas contentsCanvas(Canvas & from) const;

        /** Returns the scroll offset of the contents. 
         */
        Point scrollOffset() const {
            return scrollOffset_;
        }

        /** Updates the scroll offset of the widget. 
         
            When offset changes, the visible area must be recalculated and the widget repainted. 
         */
        virtual void setScrollOffset(Point const & value) {
            if (value != scrollOffset_) {
                scrollOffset_ = value;
                updateVisibleArea();
                repaint();
            } 
        }

        virtual void scrollBy(Point delta) {
            Point p = scrollOffset_ + delta;
            Size csize = contentsCanvasSize();
            if (p.x() < 0)
                p.setX(0);
            if (p.x() > csize.width() - width())
                p.setX(csize.width() - width());
            if (p.y() < 0)
                p.setY(0);
            if (p.y() > csize.height() - height())
                p.setY(csize.height() - height());
            setScrollOffset(p);
        } 

        virtual int getAutoWidth() const {
            return contentsRect_.width();
        }

        /** Given current width of the widget, calculates the desired height to fit all of its contents. 
         
            This function is used by the SizeHint::AutoSize to determine proper widget size. 
         */
        virtual int getAutoHeight() const {
            return contentsRect_.height();
        }

    private:

        /** Bring canvas' visible area to own scope so that it can be used by children. 
         */
        using VisibleArea = Canvas::VisibleArea;

        /** Obtains the contents visible area of the parent and then updates own and children's visible areas. 
         */
        void updateVisibleArea();

        void updateVisibleArea(VisibleArea const & parentArea, Renderer * renderer);

        /** Renderer to which the widget is attached. 
         */
        Renderer * renderer_ = nullptr;

        /** Visible area of the widget. 
         */
        Canvas::VisibleArea visibleArea_;

        /** The rectangle of the widget within its parent's client area. 
         */
        Rect rect_;

        /** The rectangle occupied by child widgets. 
         */
        Rect contentsRect_;

        /** The offset of the visible area in the contents rectangle. 
         */
        Point scrollOffset_;

        /** Visibility of the widget. 
         */
        bool visible_ = true;

        /** True if parts of the widget can be covered by other widgets that will be painted after it.
         */
        bool overlaid_ = false;

        /** The layout implementation for the widget. 
         */
        Layout * layout_ = new Layout::None{};

        SizeHint * widthHint_ = new SizeHint::AutoLayout{};
        SizeHint * heightHint_ = new SizeHint::AutoLayout{};

    //@}

    // ============================================================================================

    /** \name Painting & Style

     */
    //@{
    
    public: 

        Color const & background() const {
            return background_;
        }

        virtual void setBackground(Color value) {
            if (background_ != value) {
                background_ = value;
                repaint();
            }
        }

        Border const & border() const {
            return border_;
        }

        virtual void setBorder(Border const & value) {
            if (border_ != value) {
                border_ = value;
                repaint();
            }
        }

    protected:

        /** Paints given child. 
         
            This method is necessary because subclasses can't simply call paint() on the children because of C++ protection rules. 
            */
        void paintChild(Widget * child) {
            ASSERT(child->parent_ == this);
            child->paint();
        }

        /** Returns the attached renderer. 
         */
        Renderer * renderer() const {
            return renderer_;
        }

    private:

        Color background_ = Color::None;
        Border border_;

    //@}

    // ============================================================================================
    /** \name Generic Input 
     */
    public:

        /** When a widget is enabled it responds to input. 
         */
        bool enabled() const {
            return enabled_;
        }

        /** Enables or disables the widget
         
            Enabled widget is expected to respond to input events, while disabled widgets should ignore them. Disabled widget cannot be keyboard focused, but mouse & clipboard and selection events are still passed to the event and they should be ignored for other than visual purposes. 
         */
        virtual void setEnabled(bool value = true);

    private:

        bool enabled_ = true;

    // ========================================================================================
    /** \name Mouse Input
     */
    //@{
    public:
        VoidEvent onMouseIn;
        VoidEvent onMouseOut;
        MouseMoveEvent onMouseMove;
        MouseWheelEvent onMouseWheel;
        MouseButtonEvent onMouseDown;
        MouseButtonEvent onMouseUp;
        MouseButtonEvent onMouseClick;
        MouseButtonEvent onMouseDoubleClick;
        MouseButtonEvent onMouseTripleClick;

        bool mouseFocused() const;

        /** Returns the mouse cursor that should be used for the widget. 
         */
        MouseCursor mouseCursor() const {
            return mouseCursor_;
        }

    protected:

        /** Sets the mouse cursor of the widget.
         
            If the widget has mouse focus, updates the cursor immediately with the renderer. 
         */
        virtual void setMouseCursor(MouseCursor cursor);

        /** Called when mouse enters the widget's visible area. 
         
            Sets the renderer's mouse cursor to the widget's mouse cursor and triggers the event. 
         */
        virtual void mouseIn(VoidEvent::Payload & e);

        virtual void mouseOut(VoidEvent::Payload & e) {
            onMouseOut(e, this);
        }

        virtual void mouseMove(MouseMoveEvent::Payload & e) {
            onMouseMove(e, this);
            if (e.active() && parent_ != nullptr) {
                e->coords = parent_->toWidgetCoordinates(toRendererCoordinates(e->coords));
                parent_->mouseMove(e);
            }
        }

        virtual void mouseWheel(MouseWheelEvent::Payload & e) {
            onMouseWheel(e, this);
            if (e.active() && parent_ != nullptr) {
                e->coords = parent_->toWidgetCoordinates(toRendererCoordinates(e->coords));
                parent_->mouseWheel(e);
            }
        }

        virtual void mouseDown(MouseButtonEvent::Payload & e) {
            onMouseDown(e, this);
            if (e.active() && parent_ != nullptr) {
                e->coords = parent_->toWidgetCoordinates(toRendererCoordinates(e->coords));
                parent_->mouseDown(e);
            }
        }

        virtual void mouseUp(MouseButtonEvent::Payload & e) {
            onMouseUp(e, this);
            if (e.active() && parent_ != nullptr) {
                e->coords = parent_->toWidgetCoordinates(toRendererCoordinates(e->coords));
                parent_->mouseUp(e);
            }
        }

        virtual void mouseClick(MouseButtonEvent::Payload & e) {
            onMouseClick(e, this);
            if (e.active() && parent_ != nullptr) {
                e->coords = parent_->toWidgetCoordinates(toRendererCoordinates(e->coords));
                parent_->mouseClick(e);
            }
        }

        virtual void mouseDoubleClick(MouseButtonEvent::Payload & e) {
            onMouseDoubleClick(e, this);
            if (e.active() && parent_ != nullptr) {
                e->coords = parent_->toWidgetCoordinates(toRendererCoordinates(e->coords));
                parent_->mouseDoubleClick(e);
            }
        }

        virtual void mouseTripleClick(MouseButtonEvent::Payload & e) {
            onMouseTripleClick(e, this);
            if (e.active() && parent_ != nullptr) {
                e->coords = parent_->toWidgetCoordinates(toRendererCoordinates(e->coords));
                parent_->mouseTripleClick(e);
            }
        }

    private:
        
        /** Mouse cursor to be displayed over the widget. 
         */
        MouseCursor mouseCursor_ = MouseCursor::Default;

    //}

    // ========================================================================================
    /** \name Keyboard Input
     
     */
    //@{
    public:

        /** Determines whether the widget supports obtaining keyboard focus, or not. 
         */
        bool focusable() const {
            return focusable_;
        }

        /** Returns true if the widget currently has keyboard focus. 
         */
        bool focused() const;

        VoidEvent onFocusIn;
        VoidEvent onFocusOut;
        KeyEvent onKeyDown;
        KeyEvent onKeyUp;
        KeyCharEvent onKeyChar;

    protected:

        /** Captures keyboard input for the current widget. 
         */
        virtual void focus();

        /** Releases the keyboard input capture. 
         
            Moves to the next focusable widget, or if there is no next widget simply releases the capture. 
         */
        virtual void defocus();

        virtual void setFocusable(bool value = true);

        /** The widget has received keyboard focus. 
         */
        virtual void focusIn(VoidEvent::Payload & e) {
            onFocusIn(e, this);
            repaint();
        }

        /** The widget has lost keyboard focus. 
         */
        virtual void focusOut(VoidEvent::Payload & e) {
            onFocusOut(e, this);
            // make sure the repaint is scheduled because as of now, the widget still has the focus and will only lose it after this function returns
            requestRepaint();
        }

        virtual void keyDown(KeyEvent::Payload & e) {
            onKeyDown(e, this);
            if (e.active() && parent_ != nullptr)
                parent_->keyDown(e);
        }

        virtual void keyUp(KeyEvent::Payload & e) {
            onKeyUp(e, this);
            if (e.active() && parent_ != nullptr)
                parent_->keyUp(e);
        }

        virtual void keyChar(KeyCharEvent::Payload & e) {
            onKeyChar(e, this);
            if (e.active() && parent_ != nullptr)
                parent_->keyChar(e);
        }

    private:

        bool focusable_ = false; 

    //@}

    // ========================================================================================
    /** \name Selection & Clipboard
     */
    //@{
    public:
        StringEvent onPaste;

    protected:

        /** Triggered when previously received clipboard or selection contents are available. 
         */
        virtual void paste(StringEvent::Payload & e) {
            onPaste(e, this);
        }

        void setClipboard(std::string const & contents);

        void requestClipboardPaste();
        void requestSelectionPaste();

    //@}

    // ========================================================================================
    /** \name Helper functions
     */

    //@{

    public:
        /** Returns true if the widget is available to the user.
          
            I.e. if the widget is visible (so that it can be observed) and enabled (so that it can be interacted with).     

            This is a static function that can be conveniently used as an argument to nextWidget() and prevWidget() methods. 
         */
        static bool IsAvailable(Widget * w) {
            return w->visible() && w->enabled();
        }


    //@}

    }; // ui::Widget

} // namespace ui