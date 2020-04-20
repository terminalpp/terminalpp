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

        Layout():
            container_{nullptr} {
        }

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

        Rect align(Rect const & what, int maxWidth, HorizontalAlign hAlign) {
            switch (hAlign) {
                case HorizontalAlign::Left:
                    return Rect::FromTopLeftWH(0, what.top(), what.width(), what.height());
                case HorizontalAlign::Center:
                    return Rect::FromTopLeftWH((maxWidth - what.width()) / 2, what.top(), what.width(), what.height());
                case HorizontalAlign::Right:
                    return Rect::FromTopLeftWH(maxWidth - what.width(), what.top(), what.width(), what.height());
                default:
                    UNREACHABLE;
                    return what;
            }
        }

        Rect align(Rect const & what, int maxHeight, VerticalAlign vAlign) {
            switch (vAlign) {
                case VerticalAlign::Top:
                    return Rect::FromTopLeftWH(what.left(), 0, what.width(), what.height());
                case VerticalAlign::Middle:
                    return Rect::FromTopLeftWH(what.left(), (maxHeight - what.height()) / 2, what.width(), what.height());
                case VerticalAlign::Bottom:
                    return Rect::FromTopLeftWH(what.left(), maxHeight - what.height(), what.width(), what.height());
                default:
                    UNREACHABLE;
                    return what;
            }
        }

        /** Sets the overlay of the widget. 

            If the parent of the child is overlaid itself, then all its children are overlaid regardless of the value.  
         */
        void setChildOverlay(Widget * child, bool value) {
            // make sure that repaints will be ignored since parent relayout will trigger repaint eventually 
            child->repaintRequested_.store(true);
            child->overlaid_ = value;
        }

        virtual void requestRelayout();

    private:
        /** The container controlled by the layout. */
        Container * container_;

    };



} // namespace ui