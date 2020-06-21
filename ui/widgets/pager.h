#pragma once

#include "../container.h"

namespace ui {

    class Pager : public Container {
    public:

        Pager():
            Container{new MaximizeLayout{}} {
        }

        using Container::setWidthHint;
        using Container::setHeightHint;
        
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

    protected:
        
        /** Stops any repaints from non-active pages. 
         */
        Widget * propagatePaintTarget(Widget * sender, Widget * target) override {
            return (sender == activePage()) ? target : nullptr;
        }

        /** The pager only paints the active page. 
         */
        void paint(Canvas & canvas) override {
            Widget::paint(canvas);
            if (! children_.empty())
                paintChild(children_.back(), canvas);
        }

    }; // ui::Pager

} // namespace ui