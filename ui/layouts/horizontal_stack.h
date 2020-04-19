#pragma once

#include "../layout.h"
#include "../container.h"

namespace ui {

    class HorizontalAlign {
        Top,
        Middle,
        Bottom
    }; // ui::HorizontalAlign

    /** Horizontal Stack layout. 
     
        Stacks the widgets one on top of another using the entire width of the parent. 
     */
    class HorizontalStackLayout : public Layout {
    public:

        HorizontalAlign horizontalAlign() const {
            return hAlign_;
        }

        virtual void setHorizontalAlign(HorizontalAlign value) {
            if (hAlign_ != value) {
                hAlign_ = value;
            }
        }

    protected:

        void relayout(Container * widget) override {

        }

        /** Stacked widgets have no overlay. 
         */
        void recalculateOverlay(Container * widget) override {
            std::vector<Widget*> const & children = containerChildren(widget);
            for (auto i = children.rbegin(), e = children.rend(); i != e; ++i)
                setChildOverlay(*i, false);
        }

        HorizontalAlign hAlign_;
    }; // ui::HorizontalStackLayout



} // namespace ui