#pragma once

#include "../layout.h"
#include "../container.h"

namespace ui {

    class MaximizeLayout : public Layout {
    protected:
        void relayout(Container * widget, Size size) override {
            int autoWidth = size.width();
            int autoHeight = size.height();
            std::vector<Widget*> const & children = containerChildren(widget);
            bool overlay = false;
            for (auto i = children.rbegin(), e = children.rend(); i != e; ++i) {
                Widget * child = *i;
                if (!child->visible())
                    continue;
                int w = calculateChildWidth(child, autoWidth, autoWidth);
                int h = calculateChildHeight(child, autoHeight, autoHeight);
                resizeChild(child, w, h);
                moveChild(child, 
                    align(
                        align(Point{0,0}, w, autoWidth, HorizontalAlign::Center),
                        h, autoHeight, VerticalAlign::Middle));
                setChildOverlay(child, overlay);
                overlay = true;
            }
        }

        void recalculateOverlay(Container * widget) override {
            std::vector<Widget*> const & children = containerChildren(widget);
            bool overlay = false;
            for (auto i = children.rbegin(), e = children.rend(); i != e; ++i) {
                if ((*i)->visible()) {
                    setChildOverlay(*i, overlay);
                    overlay = true;
                }
            }
        }

    }; // ui::MaximizeLayout

} // namespace ui