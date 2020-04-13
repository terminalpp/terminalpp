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

    protected:

        friend class Container;

        virtual void relayout(Container * widget) = 0;

        /** Returns vector of children for given container. 
         */
        std::vector<Widget*> const & containerChildren(Container * container);

        void setChildRect(Widget * child, Rect const & rect) {
            child->setRect(rect);
        }

    };



} // namespace ui