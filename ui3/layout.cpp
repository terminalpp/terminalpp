#include "geometry.h"
#include "widget.h"

#include "layout.h"

namespace ui3 {

    void Layout::calculateOverlay(Widget * widget) const {
        Rect r;
        for (Widget * child : widget->children_) {
            child->overlaid_ = ! (r & child->rect()).empty();
            r = r | child->rect();
        }
    }

    Size Layout::contentsSize(Widget * widget) const {
        return widget->contentsSize();
    }

    std::deque<Widget *> const & Layout::children(Widget * widget) const {
        return widget->children_;
    }

    void Layout::resize(Widget * widget, Size const & size) const {
        if (widget->rect().size() == size) {
            if (widget->widthHint() == SizeHint::AutoSize() || widget->heightHint() == SizeHint::AutoSize())
                widget->relayout();
        } else {
            widget->resize(size);
        }
    }

    void Layout::move(Widget * widget, Point const & topLeft) const {
        widget->move(topLeft);
    }

    void Layout::Maximized::layout(Widget * widget) const {
        Rect rect{contentsSize(widget)};
        for (Widget * child : children(widget)) {
            if (!child->visible())
                continue;
            // calculate the desired width and height of the child
            int w = calculateDimension(child->widthHint(), child->rect().width(), rect.width(), rect.width());
            int h = calculateDimension(child->heightHint(), child->rect().height(), rect.height(), rect.height());
            // resize the child, which triggers its relayout and potentially updates the size too
            resize(child, w, h);
            // if the size has been updated, center the widget if needs be
            Point pos = rect.align(child->rect(), HorizontalAlign::Center, VerticalAlign::Middle);
            move(child, pos);
        }
    }




} // namespace ui3