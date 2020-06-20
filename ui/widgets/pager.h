#pragma once

#include "../container.h"

namespace ui {

    class Pager : public Container {
    public:
        
        /** Returns the currently active page. 
         */
        Widget * activePage() const {
            if (children_.empty())
                return nullptr;
            return children_.back();
        }

        /** Sets the currently active page to given widget.
         
            If the widget is not child of the pager, it is added as a child first. 
         */
        void setActivePage(Widget * widget) {
            // ivalidate the rectangle of previously visible widget
            add(widget);
        }

    }; // ui::Pager

} // namespace ui