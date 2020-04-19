#pragma once

#include "../layout.h"
#include "../container.h"

namespace ui {

    enum class HorizontalAlign {
        Top,
        Middle,
        Bottom
    }; // ui::HorizontalAlign

    /** Horizontal Stack layout. 
     
        Stacks the widgets one on top of another using the entire width of the parent. 
     */
    class HorizontalStackLayout : public Layout {
    public:

        explicit HorizontalStackLayout(HorizontalAlign hAlign = HorizontalAlign::Top):
            hAlign_{hAlign} {
        }

        HorizontalAlign horizontalAlign() const {
            return hAlign_;
        }

        virtual void setHorizontalAlign(HorizontalAlign value) {
            if (hAlign_ != value) {
                hAlign_ = value;
            }
        }

    protected:

        int getStartY(int actualHeight, int fullHeight) {
            switch (hAlign_) {
                case HorizontalAlign::Top:
                    return 0;
                case HorizontalAlign::Middle:
                    return (fullHeight - actualHeight) / 2;
                case HorizontalAlign::Bottom:
                    return fullHeight - actualHeight;
            }
        }

        void relayout(Container * widget, Canvas const & contentsCanvas) override {
            std::vector<Widget*> const & children = containerChildren(widget);
            ASSERT(! children.empty());
            int autoWidth = contentsCanvas.width();
            int availableHeight = contentsCanvas.height();
            // determine fixed sized widgets and remove them from the available height
            // determine the actual height that will be used and set the width of the child to the autoWidth
            int autoElements = 0;
            for (Widget * child : children) {
                int w = calculateChildWidth(child, autoWidth, autoWidth);
                setChildRect(child, Rect::FromTopLeftWH(0,0, w, child->height()));
                // now that the width of the child is correct, we determine its height if fixed
                if (child->heightHint() == SizeHint::Kind::Fixed)
                    availableHeight -= child->height();
                else if (child->heightHint() == SizeHint::Kind::Auto)
                    ++autoElements;
            }
            int autoHeight = availableHeight / static_cast<int>(children.size());
            // once we know the available height, calculate the actual height by doing a dry run
            int actualHeight = 0;
            for (Widget * child : children)
                actualHeight += calculateChildHeight(child, autoHeight, availableHeight);
            // determine the difference if the autoHeight elements won't fit the whole height precisely (and if there are elements whose size can actually be adjusted)
            int diff = (autoElements != 0 && availableHeight > actualHeight) ? availableHeight - actualHeight : 0;
            actualHeight += diff;
            // get the top placement
            int top = getStartY(actualHeight, contentsCanvas.height());
            // and finally set the widget sizes appropriately, centering them vertically, apply the diff to the first non-fixed element
            for (Widget * child : children) {
                int w = child->width();
                int h = calculateChildHeight(child, autoHeight, availableHeight);
                if (diff > 0 && child->heightHint() == SizeHint::Kind::Auto) {
                    h += diff;
                    diff = 0;
                }
                Rect r{Rect::FromTopLeftWH(0, top, w, h)};
                r = centerHorizontally(r, autoWidth);
                setChildRect(child, r);
                setChildOverlay(child, false);
                top += h;
            }
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