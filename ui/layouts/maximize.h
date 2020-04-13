#pragma once

#include "../layout.h"
#include "../container.h"

namespace ui {

    class MaximizeLayout : public Layout {
    protected:
        void relayout(Container * widget) override {
            std::vector<Widget*> const & children = containerChildren(widget);
            Rect rect{Rect::FromWH(widget->width(), widget->height())};
            for (Widget * child : children) {
                setChildRect(child, rect);
                setChildOverlay(child, true);
            }
            if (! children.empty())
                setChildOverlay(children.back(), false);
        }

        void recalculateOverlay(Container * widget) override {
            if (isOverlaid(widget)) 
                return Layout::recalculateOverlay(widget);
            std::vector<Widget*> const & children = containerChildren(widget);
            bool overlay = false;
            for (auto i = children.rbegin(), e = children.rend(); i != e; ++i) {
                setChildOverlay(*i, overlay);
                overlay = true;
            }
        }

    }; // ui::MaximizeLayout

} // namespace ui