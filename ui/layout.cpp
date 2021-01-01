#include "geometry.h"
#include "widget.h"

#include "layout.h"

namespace ui {

    int SizeHint::Manual::calculateWidth(Widget const * widget, int autoSize, int availableSize) const {
        MARK_AS_UNUSED(autoSize);
        MARK_AS_UNUSED(availableSize);
        return widget->width();
    }
    int SizeHint::Manual::calculateHeight(Widget const * widget, int autoSize, int availableSize) const {
        MARK_AS_UNUSED(autoSize);
        MARK_AS_UNUSED(availableSize);
        return widget->height();
    }

    int SizeHint::AutoSize::calculateWidth(Widget const * widget, int autoSize, int availableSize) const {
        MARK_AS_UNUSED(autoSize);
        MARK_AS_UNUSED(availableSize);
        return widget->getAutoWidth();
    }

    int SizeHint::AutoSize::calculateHeight(Widget const * widget, int autoSize, int availableSize) const {
        MARK_AS_UNUSED(autoSize);
        MARK_AS_UNUSED(availableSize);
        return widget->getAutoHeight();
    }

    void Layout::calculateOverlay(Widget * widget) const {
        Rect r;
        auto kids = children(widget);
        // use reverse iterator to start with kids that are last
        for (auto i = kids.rbegin(), e = kids.rend(); i != e; ++i) {
            Widget * child = *i;
            setOverlaid(child, ! (r & child->rect()).empty());
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
            if (widget->widthHint()->isAuto() || widget->heightHint()->isAuto())
                widget->relayout();
        } else {
            widget->resize(size);
        }
    }

    void Layout::move(Widget * widget, Point const & topLeft) const {
        widget->move(topLeft);
    }

    void Layout::setOverlaid(Widget * widget, bool value) const {
        widget->overlaid_ = value;
    }

    // Maximized

    void Layout::Maximized::layout(Widget * widget) const {
        Rect rect{contentsSize(widget)};
        for (Widget * child : children(widget)) {
            if (!child->visible())
                continue;
            // calculate the desired width and height of the child
            int w = child->widthHint()->calculateWidth(child, rect.width(), rect.width());
            int h = child->heightHint()->calculateHeight(child, rect.height(), rect.height());
            // resize the child, which triggers its relayout and potentially updates the size too
            resize(child, w, h);
            // if the size has been updated, center the widget if needs be
            Point pos = rect.align(child->rect(), HorizontalAlign::Center, VerticalAlign::Middle);
            move(child, pos);
        }
    }

    void Layout::Maximized::calculateOverlay(Widget * widget) const {
        bool overlaid = false;
        auto kids = children(widget);
        // use reverse iterator to start with kids that are last
        for (auto i = kids.rbegin(), e = kids.rend(); i != e; ++i) {
            if ((*i)->visible()) {
                setOverlaid(*i, overlaid);
                overlaid = true;
            }
        }        
    }

    // Row

    void Layout::Row::layout(Widget * widget) const {
        Rect rect{contentsSize(widget)};
        // iterate over the children, determine how many autosized widgets there are and what is the auto width budget
        int availWidth = rect.width();
        int autoWidgets = 0;
        for (Widget * child : children(widget)) {
            if (! child->visible())
                continue;
            // if autolayouted, increase number of widgets and continue
            if (child->widthHint()->isLayout()) {
                ++autoWidgets;
                continue;
            }
            // otherwise update the size already
            // calculate the desired width and height of the child
            int w = child->widthHint()->calculateWidth(child, 0, rect.width());
            int h = child->heightHint()->calculateHeight(child, rect.height(), rect.height());
            // resize the child, which triggers its relayout and potentially updates the size too
            resize(child, w, h);
            // update the available height
            availWidth -= child->rect().width();
        }
        // iterate over the layout-sized children and resize them, get the total width. The last children fills the rest of space
        int totalWidth = rect.width() - availWidth;
        if (autoWidgets > 0) {
            int autoWidth = availWidth / autoWidgets;
            for (Widget * child : children(widget)) {
                // ignore invisible and non-layout sized widgets
                if (!child->visible() || ! child->widthHint()->isLayout())    continue;
                int h = child->heightHint()->calculateHeight(child, rect.height(), rect.height());
                resize(child, autoWidgets > 1 ? autoWidth : availWidth, h);
                availWidth -= autoWidth;
                --autoWidgets;
                totalWidth += child->rect().width();
            }
        }
        // finally the widgets are to be moved - based on the horizontal align, determine where to start putting them
        int left = 0;
        switch (hAlign_) {
            case HorizontalAlign::Left:
                break;
            case HorizontalAlign::Center:
                left = (rect.width() - totalWidth) / 2;
                break;
            case HorizontalAlign::Right:
                left = rect.width() - totalWidth;
                break;
        }
        // then align each widget horizontally
        for (Widget * child : children(widget)) {
            move(child, rect.align(Rect{Point{left, 0}, child->rect().size()}, vAlign_));
            left += child->rect().width();
        }
    }

    void Layout::Row::calculateOverlay(Widget * widget) const {
        for (Widget * child : children(widget))
            setOverlaid(child, false);
    }


    // Column

    void Layout::Column::layout(Widget * widget) const {
        Rect rect{contentsSize(widget)};
        // iterate over the children, determine how many autosized widgets there are and what is the auto height budget
        int availHeight = rect.height();
        int autoWidgets = 0;
        for (Widget * child : children(widget)) {
            if (! child->visible())
                continue;
            // if autolayouted, increase number of widgets and continue
            if (child->heightHint()->isLayout()) {
                ++autoWidgets;
                continue;
            }
            // otherwise update the size already
            // calculate the desired width and height of the child
            int w = child->widthHint()->calculateWidth(child, rect.width(), rect.width());
            int h = child->heightHint()->calculateHeight(child, 0, rect.height());
            // resize the child, which triggers its relayout and potentially updates the size too
            resize(child, w, h);
            // update the available height
            availHeight -= child->rect().height();
        }
        // iterate over the layout-sized children and resize them, get the total height. The last children fills the rest of space
        int totalHeight = rect.height() - availHeight;
        if (autoWidgets > 0) {
            int autoHeight = availHeight / autoWidgets;
            for (Widget * child : children(widget)) {
                // ignore invisible and non-layout sized widgets
                if (!child->visible() || ! child->heightHint()->isLayout())    continue;
                int w = child->widthHint()->calculateWidth(child, rect.width(), rect.width());
                resize(child, w, autoWidgets > 1 ? autoHeight : availHeight);
                availHeight -= autoHeight;
                --autoWidgets;
                totalHeight += child->rect().height();
            }
        }
        // finally the widgets are to be moved - based on the vertical align, determine where to start putting them
        int top = 0;
        switch (vAlign_) {
            case VerticalAlign::Top:
                break;
            case VerticalAlign::Middle:
                top = (rect.height() - totalHeight) / 2;
                break;
            case VerticalAlign::Bottom:
                top = rect.height() - totalHeight;
                break;
        }
        // then align each widget horizontally
        for (Widget * child : children(widget)) {
            move(child, rect.align(Rect{Point{0, top}, child->rect().size()}, hAlign_));
            top += child->rect().height();
        }
    }

    void Layout::Column::calculateOverlay(Widget * widget) const {
        for (Widget * child : children(widget))
            setOverlaid(child, false);
    }



} // namespace ui