#pragma once

#include "../layout.h"
#include "../container.h"

namespace ui {

    class MaximizeLayout : public Layout {
    protected:
        void relayout(Container * widget, Canvas const & contentsCanvas) override {
            int autoWidth = contentsCanvas.width();
            int autoHeight = contentsCanvas.height();
            std::vector<Widget*> const & children = containerChildren(widget);
            for (Widget * child : children) {
                if (!child->visible())
                    continue;
                int w = calculateChildWidth(child, autoWidth, autoWidth);
                int h = calculateChildHeight(child, autoHeight, autoHeight);
                Rect r{Rect::FromWH(w, h)};
                r = align(r, autoWidth, HorizontalAlign::Center);
                r = align(r, autoHeight, VerticalAlign::Middle);
                setChildRect(child, r);
                setChildOverlay(child, true);
            }
            if (! children.empty())
                setChildOverlay(children.back(), false);
        }

        void recalculateOverlay(Container * widget) override {
            std::vector<Widget*> const & children = containerChildren(widget);
            bool overlay = false;
            for (auto i = children.rbegin(), e = children.rend(); i != e; ++i) {
                setChildOverlay(*i, overlay);
                overlay = true;
            }
        }

    }; // ui::MaximizeLayout

} // namespace ui