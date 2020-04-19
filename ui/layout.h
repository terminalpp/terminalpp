#pragma once

#include <vector>

#include "widget.h"

namespace ui {

    class Container;

    /** Basic API for implementing layouts. 
     
        The layout is responsible for updating the size and position of container's children widgets.
     */
    class Layout {
    public:
        /** The default layout
         */
        static Layout * None;

        virtual ~Layout() { 
        }

    protected:

        friend class Container;

        /** Recalculates the dimensions and layout of the children. 
         */
        virtual void relayout(Container * widget, Canvas const & contentsCanvas) = 0;

        /** Calculates the actual overlay of the children in the given container. 
         
            This method is to be called when the layout of the children is valid, but the overlay of the parent has changed. 
         */
        virtual void recalculateOverlay(Container * widget);

        /** Returns vector of children for given container. 
         */
        std::vector<Widget*> const & containerChildren(Container * container);

        /** Sets the rectangle of the children. 
         */
        void setChildRect(Widget * child, Rect const & rect) {
            // make sure that repaints will be ignored since parent relayout will trigger repaint eventually 
            child->repaintRequested_.store(true);
            child->setRect(rect);
        }

        int calculateChildWidth(Widget * child, int autoWidth, int availableWidth) {
            return child->widthHint().calculateSize(child->width(), autoWidth, availableWidth);
        }

        int calculateChildHeight(Widget * child, int autoHeight, int availableHeight) {
            return child->heightHint().calculateSize(child->height(), autoHeight, availableHeight);
        }

        Rect centerHorizontally(Rect const & what, int width) {
            return what + Point{(width - what.width()) / 2, 0};
        }

        Rect centerVertically(Rect const & what, int height) {
            return what + Point{0, (height - what.height()) / 2};
        }

        bool isOverlaid(Widget * widget) const {
            return widget->overlaid_;
        }

        /** Sets the overlay of the widget. 

            If the parent of the child is overlaid itself, then all its children are overlaid regardless of the value.  
         */
        void setChildOverlay(Widget * child, bool value) {
            // make sure that repaints will be ignored since parent relayout will trigger repaint eventually 
            child->repaintRequested_.store(true);
            child->overlaid_ = value;
        }

    };



} // namespace ui