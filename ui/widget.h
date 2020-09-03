#pragma once

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
    public:

        Widget() {
        }

        virtual ~Widget() {
            for (Widget * child : children_)
                delete child;
            // layout is owned
            delete layout_;
        }

    // ============================================================================================
    /** \name Event Scheduling
     */
    //@{
#ifndef NDEBUG 
    public: 

        static std::thread::id UIThreadID() {
            static std::thread::id id{std::this_thread::get_id()};
            return id;
        }
#endif


    protected:
        void schedule(std::function<void()> event);

    private:
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

        SizeHint widthHint() const {
            return widthHint_;
        }

        virtual void setWidthHint(SizeHint const & value) {
            if (widthHint_ != value) {
                widthHint_ = value;
                if (parent_ != nullptr)
                    parent_->relayout();
            }
        }

        SizeHint heightHint() const {
            return heightHint_;
        }

        virtual void setHeightHint(SizeHint const & value) {
            if (heightHint_ != value) {
                heightHint_ = value;
                if (parent_ != nullptr)
                    parent_->relayout();
            }
        }

        /** Moves the widget within its parent. 
         */
        virtual void move(Point const & topLeft);

        /** Resizes the widget. 
         */
        virtual void resize(Size const & size);

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

        /** Returns the contents size. 
         */
        virtual Size contentsSize() const {
            return rect_.size();
        }

        /** Returns the canvas used for the contents of the widget. 
         
            The canvas has the contentsSize() size and is scrolled according to the scrollOffset().
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
            Size csize = contentsSize();
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

        /** Returns the hint about the contents size of the widget. 
         
            Depending on the widget's size hints returns the width and height the widget should have when autosized. 

            */
        virtual Size getAutosizeHint() {
            if (widthHint_ == SizeHint::AutoSize() || heightHint_ == SizeHint::AutoSize()) {
                Rect r;
                for (Widget * child : children_) {
                    if (!child->visible())
                        continue;
                    r = r | child->rect_;
                }
                return Size{
                    widthHint_ == SizeHint::AutoSize() ? r.width() : rect_.width(),
                    heightHint_ == SizeHint::AutoSize() ? r.height() : rect_.height()
                };
            } else {
                return rect_.size();
            }
        }

        void relayout();

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

        /** The offset of the visible area in the contents rectangle. 
         */
        Point scrollOffset_;

        /** Visibility of the widget. 
         */
        bool visible_ = true;

        /** If true, the widget's relayout should be called after its parent relayout happens. Calling resize 
         */
        //bool pendingRelayout_ = false;

        /** True if the widget is currently being relayouted. 
         */
        bool relayouting_ = false;

        /** True if parts of the widget can be covered by other widgets that will be painted after it.
         */
        bool overlaid_ = false;

        /** The layout implementation for the widget. 
         */
        Layout * layout_ = new Layout::None{};

        SizeHint widthHint_;
        SizeHint heightHint_;

    //@}

    // ============================================================================================

    /** \name Painting & Style
     */
    //@{
    
    public: 

        /** Repaints the widget. 
         
            Triggers the repaint of the widget. 
         */
        void repaint();

        /** Schedules repaint. 
         
            This method can be called from a different thread. 
         */
        void scheduleRepaint();

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

        /** Simple RAII widget lock that when engaged prevents widget from repainting. 
         */
        class Lock {
        public:
            Lock(Widget * widget):
                widget_{widget} {
                ++widget_->lock_;
            }

            ~Lock() {
                if (--widget_->lock_ == 0 && widget_->pendingRepaint_ == true) {
                    widget_->requestRepaint();
                }
            }
        private:
            Widget * widget_;
        }; 

        /** Returns whether the widget is locked or not. 
         
            A locked widget will not repaint itself.
         */
        bool locked() const {
            return lock_ > 0;
        }

        /** Called when repaint of the widget is requested. 

            Widget subclasses can override this method to delegate the repaint event to other widgets up the tree, such as in cases of transparent background widgets. 
         */
        virtual void requestRepaint();

        /** Paints given child. 
         
            This method is necessary because subclasses can't simply call paint() on the children because of C++ protection rules. 
            */
        void paintChild(Widget * child) {
            ASSERT(child->parent_ == this);
            child->paint();
        }

        /** Immediately paints the widget. 
         
            This method is to be used when another widget is to be painted as part of its parent. It clears the pending repaint flag, unlocking future repaints of the widgets, creates the appropriate canvas and calls the paint(Canvas &) method to actually draw the widget. 

            To explicitly repaint the widget, the repaint() method should be called instead, which optimizes the number of repaints and tells the renderer to repaint the widget.                 
            */        
        void paint();

        /** Returns the attached renderer. 
         */
        Renderer * renderer() const {
            return renderer_;
        }

        /** Determines whether a paint request in the given child's subtree is to be allowed or not. 
         
            Returns true if the request is to be allowed, false if the repaint is not necessary (such as the child will nevet be displayed, or parent has already scheduled its own repaint and so will repaint the child as well). 
            
            When blocking the child repaint, a parent has the option to perform its own repaint instead. 
            */
        virtual bool allowRepaintRequest(Widget * immediateChild);

        /** Actual paint method. 
         
            Override this method in subclasses to actually paint the widget's contents using the provided canvas. The default implementation simply paints the widget's children. 
            */
        virtual void paint(Canvas & canvas) {
            MARK_AS_UNUSED(canvas);
            for (Widget * child : children_) {
                if (child->visible())
                    child->paint();
            }
        }

    private:

        friend class Lock;

        /** Since widget's start detached, their paint is blocked by setting pending repaint to true. When attached, and repainted via its parent, the flag will be cleared. 
         */
        std::atomic<bool> pendingRepaint_ = true;

        unsigned lock_ = 0;

        Color background_;
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

    protected:

        virtual void mouseIn(VoidEvent::Payload & e) {
            onMouseIn(e, this);
        }

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
            repaint();
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
            return w->enabled() && w->enabled();
        }


    //@}

    }; // ui::Widget

} // namespace ui