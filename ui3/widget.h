#pragma once

#include <atomic>
#include <deque>

#include "helpers/helpers.h"

#include "canvas.h"
#include "layout.h"

namespace ui3 {

    class Widget;

    /** Base class for all ui widgets. 
     
     */
    class Widget {
        friend class Renderer;
        friend class Layout;
    public:

        Widget() {
        }

        virtual ~Widget() {
            for (Widget * child : children_)
                delete child;
            // layout is owned
            delete layout_;
        }
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

        private:
            /** Mutex protecting the renderer pointer in the visible area for the widget. 
             */
            std::mutex rendererGuard_;

            Widget * parent_;
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

            SizeHint heightHint() const {
                return heightHint_;
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

            virtual void setWidthHint(SizeHint const & value) {
                if (widthHint_ != value) {
                    widthHint_ = value;
                    if (parent_ != nullptr)
                        parent_->relayout();
                }
            }

            virtual void setHeightHint(SizeHint const & value) {
                if (heightHint_ != value) {
                    heightHint_ = value;
                    if (parent_ != nullptr)
                        parent_->relayout();
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

        /** \name Painting 
         */
        //@{
        
        /** Repaints the widget. 
         */
        public: 

            virtual void repaint();

        protected:

            /** Immediately paints the widget. 
             
                This method is to be used when another widget is to be painted as part of its parent. It clears the pending repaint flag, unlocking future repaints of the widgets, creates the appropriate canvas and calls the paint(Canvas &) method to actually draw the widget. 

                To explicitly repaint the widget, the repaint() method should be called instead, which optimizes the number of repaints and tells the renderer to repaint the widget.                 
             */        
            void paint() {
                pendingRepaint_ = false;
                Canvas canvas{visibleArea_, contentsSize()};
                paint(canvas);
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

        //@}

        // ========================================================================================
        /** \name Mouse Input
         */

        // ========================================================================================
        /** \name Keyboard Input
         */

        // ========================================================================================
        /** \name Selection & Clipboard
         */
        

    }; // ui::Widget

} // namespace ui