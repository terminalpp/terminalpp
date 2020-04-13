#pragma once

#include "../layout.h"
#include "../container.h"

namespace ui {

    class MaximizeLayout : public Layout {
    protected:
        void relayout(Container * widget) override {
            Rect rect{Rect::FromWH(widget->width(), widget->height())};
            for (Widget * child : containerChildren(widget))
                setChildRect(child, rect);
        }

    }; // ui::MaximizeLayout

} // namespace ui