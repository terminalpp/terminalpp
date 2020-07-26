#pragma once

#include <atomic>
#include <deque>

#include "helpers/helpers.h"

#include "events.h"
#include "canvas.h"
#include "layout.h"

namespace ui {

    class Widget;

    /** Base class for all ui widgets. 
     
     */
    class Widget {
        friend class Renderer;
        friend class Layout;
        template<typename T>
        friend class EventQueue;
    public:

        Widget() {
        }

        virtual ~Widget() {
            for (Widget * child : children_)
                delete child;
            // layout is owned
            delete layout_;
        }

        // TODO determine where enabled goes and what precisely it is

        bool enabled() const {
            return enabled_;
        }

        // TODO setEnabled

    private:

        bool enabled_ = true;


        // ========================================================================================
        /** \name Event Scheduling
         */
        //@{
        protected:
            void schedule(std::function<void()> event);

        private:
            size_t pendingEvents_{0};

        //@}

        // ========================================================================================

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
                ASSERT(visibleArea_.attached());
                return rendererCoords - visibleArea_.offset();
            }

            /** Given widget coordinates, returns those coordinates relative to the renderer's area (the window). 
             
                Can only be called for widgets which are attached to a renderer, translates the coordinates irrespectively of whether they are visible in the window, or not. 
             */
            Point toRendererCoordinates(Point widgetCoords) const {
                ASSERT(visibleArea_.attached());
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

        protected:

            /** Adds given widget as a child so that it will be painted first. 
             */
            void attach(Widget * child);

            /** Adds given widget as so that it will be painted last. 
             */
            void attachBack(Widget * child);

            /** Removes given widget from the child widgets. 
             */
            virtual void detach(Widget * child);

            /** Returns true if the widget is a root widget, i.e. if it has no parent *and* is attached to a renderer. 
             */
            bool isRootWidget() const {
                return (parent_ == nullptr) && visibleArea_.attached();
            }

            std::deque<Widget *> const & children() const {
                return children_;
            }

        private:
            /** Mutex protecting the renderer pointer in the visible area for the widget. 
             */
            std::mutex rendererGuard_;

            Widget * parent_ = nullptr;
            std::deque<Widget *> children_;


        //@}

        // ========================================================================================

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

            /** Returns the rectangle the widget occupies in its parent's contents area. 
             */
            Rect const & rect() const {
                return rect_;
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

            void updateVisibleArea(VisibleArea const & parentArea);

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

        // ========================================================================================

        /** \name Painting & Style
         */
        //@{
        
        public: 

            /** Repaints the widget. 
             
                Triggers the repaint of the widget. 

                Widget subclasses can override this method to delegate the repaint event to other widgets up the tree, such as in cases of transparent background widgets. 
             */
            virtual void repaint();

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

            /** Immediately paints the widget. 
             
                This method is to be used when another widget is to be painted as part of its parent. It clears the pending repaint flag, unlocking future repaints of the widgets, creates the appropriate canvas and calls the paint(Canvas &) method to actually draw the widget. 

                To explicitly repaint the widget, the repaint() method should be called instead, which optimizes the number of repaints and tells the renderer to repaint the widget.                 
             */        
            void paint() {
                pendingRepaint_ = false;
                Canvas canvas{visibleArea_, contentsSize()};
                // paint the background first
                canvas.setBg(background_);
                canvas.fill(canvas.rect());
                // now paint whatever the widget contents is
                paint(canvas);
                // TODO paint the border now that the widget has been painted
                // TODO 
            }

            /** Returns the attached renderer. 
             */
            Renderer * renderer() const {
                return visibleArea_.renderer();
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

            /** Since widget's start detached, their paint is blocked by setting pending repaint to true. When attached, and repainted via its parent, the flag will be cleared. 
             */
            bool pendingRepaint_ = true;

            Color background_;
            Border border_;

        //@}

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

            virtual void setFocusable(bool value = true);

            virtual void focusIn(VoidEvent::Payload & e) {
                onFocusIn(e, this);
            }

            virtual void focusOut(VoidEvent::Payload & e) {
                onFocusOut(e, this);
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

            bool focusable_ = true; // TODO should this be false? 

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

    }; // ui::Widget

} // namespace ui