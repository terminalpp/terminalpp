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
        virtual void relayout(Container * widget, Size size) = 0;

        /** Calculates the actual overlay of the children in the given container. 
         
            This method is to be called when the layout of the children is valid, but the overlay of the parent has changed. 
         */
        virtual void recalculateOverlay(Container * widget);

        /** Returns vector of children for given container. 
         */
        std::vector<Widget*> const & containerChildren(Container * container);

        /** Updates the size of given child. 
         
            Calls the resize() method on the child widget. If the new size is identical as the current size of the child, but there is a relayout pending relayouts the child immediately (this is what the resize would have done anyways if the size changed).

            This prevents the odd behavior of adding autosized zero sized child to zero sized container, if which case the autosize on the child would not be updated as the container would set its size to already zero. However when widgets are added to containers, their layout is requested (Container::Add) and this method makes sure they will be relayouted in such case, thus propagating the autosize changes. 
         */
        void resizeChild(Widget * child, int width, int height) {
            // TODO explain why the relayout - and relayout when added to a child
            if (child->width() != width || child->height() != height) {
                child->resize(width, height);
            } else if (child->pendingRelayout_)
                child->calculateLayout();
        }

        void moveChild(Widget * child, Point topLeft) {
            child->move(topLeft);
        }

        int calculateChildWidth(Widget * child, int autoWidth, int availableWidth) {
            return child->widthHint().calculateSize(child->width(), autoWidth, availableWidth);
        }

        int calculateChildHeight(Widget * child, int autoHeight, int availableHeight) {
            return child->heightHint().calculateSize(child->height(), autoHeight, availableHeight);
        }

        Point align(Point const & x, int width, int maxWidth, HorizontalAlign hAlign) {
            switch (hAlign) {
                case HorizontalAlign::Left:
                    return x;
                case HorizontalAlign::Center:
                    return x + Point{(maxWidth - width) / 2, 0};
                case HorizontalAlign::Right:
                    return x + Point{maxWidth - width, 0};
                default:
                    UNREACHABLE;
                    return x;
            }
        }

        Point align(Point const & x, int height, int maxHeight, VerticalAlign vAlign) {
            switch (vAlign) {
                case VerticalAlign::Top:
                    return x;
                case VerticalAlign::Middle:
                    return x + Point{0, (maxHeight - height) / 2};
                case VerticalAlign::Bottom:
                    return x + Point{0, maxHeight - height};
                default:
                    UNREACHABLE;
                    return x;
            }
        }

        /** Sets the overlay of the widget. 

            If the parent of the child is overlaid itself, then all its children are overlaid regardless of the value so when overlaid changes, the child must be relayouted as well to propagate this change. 

            TODO This means that we have multiple children overlays per relayout since the resize can relayout first, and then another relayout will occur if the overlay changes. This can be simplified by buffering the overlays on the widgets. 
         */
        void setChildOverlay(Widget * child, bool value) {
            value = value || child->parent()->overlaid_; 
            if (child->overlaid_ != value) {
                child->overlaid_ = value;
                child->pendingRelayout_ = true;
                child->calculateLayout();
            }
        }

        /** Requests the attached container to relayout itself. 
         
            If the layout has properties that alter the layout, changing these should trigger the relayout of the registered container by calling this method. 
         */
        virtual void requestRelayout();

    private:

        /** The container controlled by the layout. */
        Container * container_;
    };

} // namespace ui