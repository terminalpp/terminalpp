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
    public:

        virtual ~Widget() {
            for (Widget * child : children_)
                delete child;
        }

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

            bool isDominatedBy(Widget * widget) const {
                NOT_IMPLEMENTED;
            }

            Widget * commonParentWith(Widget * other) const {
                NOT_IMPLEMENTED;
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

        private:

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

            /** Moves the widget within its parent. 
             */
            virtual void move(Point const & topLeft);

            /** Resizes the widget. 
             */
            virtual void resize(Size const & size);

        protected:

            /** Bring canvas' visible area to own scope so that it can be used by children. 
             */
            using VisibleArea = Canvas::VisibleArea;

            /**
                # Example Scenarios

                Without any autosizing the layout process with a parent and child proceeds in the following way:

                - (p1) parent's relayout() is invoked, sets layouting_ to true to indicate relayout in progress
                - (p2) parent relayouts its contents, this calls move() and resize() of the child, which would trigger relayout of the parent (but since layouting is in progress, these are no-ops, but they fire the respective events), calling resize() on the child sets its pendingRelayout_ flag
                - (p3) parent sets pendingRelayout_ to false
                - (p4) parent calls relayout() on any children than have pendingRelayout_ flag on (there is no autosizing so these will not re-invoke parents' relayout() method)
                - (c1) child's relayout() is invoked, sets the layouting_ flag
                - (c2-4) identical to **p2-4**
                - (c5) child checks pendingRelayout_, which is  (i.e. no resizes of its children in **c4**, so the layout is valid)
                - (c6) child checks its autosize, but no change, so layout is valid
                - (c7) 
                - (c6) child checks pendingRelayout_ (if there was any that means relayouting its children )
                - (p5) parent checks its getAutosizeHint() method, which in the absence of autosizing returns current size, so no resize of parent will happen
                - (p6) parent  


                With widget whose child autosizes, such as a label that has its width determined by layout, but height determined by the text itself, and with the parent widget being autosized too (height to be that of its children), the following happens:

                - (p1) parent's relayout is invoked, sets layouting_) to true to indicate relayout in progress
                - (p2) parent relayouts its child widgets using its layout
                - (pl1) the layout determines that child's height is autosized and will not update it, but updates width according to the layout, calling resize() on the child
                - (cr1) the resize calls relayout of parent (since parent relayouts, this does nothing )


                Without autosize, for top widget (i.e. resize triggered by user) is simple:

                1) set layouting_ to true
                2) relayout itself (resize & move children, resized children set their pendingRelayout_ flags)
                3) set pendingRelayout_ to false
                4) relayout children that have pending relayouts (none of these change their size, so they will not attempt to trigger relayout in parent)
                5) since the widget itself does not do autosize, it is not resized itself, so no parent relayout happens
                6) since there were no parent relayouts triggered in 4), we are done with relayouting
                7) parent is not relayouting itself (i.e. we are the root), so set layouting_ to false
                8) our size and children's layout are correct (getAutosizeHint() returns current size)
                9) update visible areas transitively
                10) repaint

                From child's point of view, whose resize was triggered by parent relayout, the following happens:

                1) set layouting_ to true
                2-6) like previous example
                7) parent is relayouting (our relayout was triggered from its 4), set layouting_ to false and don't do anything, the root of relayout will do the visible area update & repaint

                If the child has autoresize property *and* gets resized during the layout, then the following happens:

                1-3) on parent are identical as in the first case
                then child gets relayouted, which in its stem 

                - then do child that has autoresize
                - and finally parent that has autoresize

            
            */
            void relayout();

            /** Returns the hint about the contents size of the widget. 
             
                Depending on the widget's size hints returns the width and height the widget should have when autosized. 

             */
            virtual Size getAutosizeHint() {
                NOT_IMPLEMENTED;
            }

            /** Returns the contents visible area of the widget. 
             
                Default implementation returns the visible area of the widget itself. However, this method can be overriden to provide specific behavior for widgets supporting scrolling, borders, etc. 
             */
            virtual VisibleArea getContentsVisibleArea() {
                return visibleArea_;
            }
        
        private:

            /** Obtains the contents visible area of the parent and then updates own and children's visible areas. 
             */
            void updateVisibleArea();

            /** Given a contents visible area of the parent, updates own visible area and that of its children. 
             */
            void updateVisibleArea(Canvas::VisibleArea const & parent) {
                visibleArea_ = parent.clip(rect_);
                Canvas::VisibleArea contentsArea = getContentsVisibleArea();
                for (Widget * child : children_)
                    child->updateVisibleArea(contentsArea);
            }

            /** Visible area of the widget. 
             */
            Canvas::VisibleArea visibleArea_;

            /** The rectangle of the widget within its parent's client area. 
             */
            Rect rect_;

            /** Visibility of the widget. 
             */
            bool visible_;

            /** If true, the widget's relayout should be called after its parent relayout happens. Calling resize 
             */
            bool pendingRelayout_;

            /** True if the widget is currently being relayouted. 
             */
            bool relayouting_;

            /** True if parts of the widget can be covered by other widgets that will be painted after it.
             */
            bool overlaid_;

            /** The layout implementation for the widget. 
             */
            Layout * layout_;

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
                pendingRepaint_.clear();
                Canvas canvas{visibleArea_};
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

        private:

            /** Actual paint method. 
             
                Override this method in subclasses to actually paint the widget's contents using the provided canvas. 
             */
            virtual void paint(Canvas & canvas) = 0;

            std::atomic_flag pendingRepaint_;

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