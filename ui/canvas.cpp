#include "canvas.h"
#include "widget.h"
#include "renderer.h"

namespace ui {

    Canvas::Canvas(Widget * widget):
        width_{widget->rect_.width()},
        height_{widget->rect_.height()},
        buffer_{widget->renderer()->buffer()},
        visibleRect_{widget->visibleRect_},
        bufferOffset_{widget->bufferOffset_} {
    }

    Canvas & Canvas::drawBuffer(Buffer const & buffer, Point topLeft) {
        // calculate the target rectangle in the canvas and its intersection with the visible rectangle and offset it to the backing buffer coordinates
        Rect r = (Rect::FromTopLeftWH(topLeft, buffer.width(), buffer.height()) & visibleRect_) + bufferOffset_;
        // calculate the buffer offset for the input buffer
        Point bufferOffset = topLeft + bufferOffset_;
        for (int row = r.top(), re = r.bottom(); row < re; ++row) {
            for (int col = r.left(), ce = r.right(); col < ce; ++col) {
                buffer_.at(col, row) = buffer.at(col - bufferOffset.x(), row - bufferOffset.y());
            }
        }
        return *this;
    }

    Canvas & Canvas::fillRect(Rect const & rect) {
        Rect r = (rect & visibleRect_) + bufferOffset_;
        for (int row = r.top(), re = r.bottom(); row < re; ++row) {
            for (int col = r.left(), ce = r.right(); col < ce; ++col) {
                applyBrush(buffer_.at(col, row), bg_);
            }
        }
        return *this;
    }

    Canvas & Canvas::drawBorder(Border const & border, Point from, Point to) {
        if (from.x() == to.x()) {
            int inc = (from.y() < to.y()) ? 1 : -1;
            for (; from != to; from.setY(from.y() + inc)) {
                Cell * c = at(from);
                if (c != nullptr)
                    c->setBorder(c->border().updateWith(border));
            }
        } else if (from.y() == to.y()) {
            int inc = (from.x() < to.x()) ? 1 : -1;
            for (; from != to; from.setX(from.x() + inc)) {
                Cell * c = at(from);
                if (c != nullptr)
                    c->setBorder(c->border().updateWith(border));
            }
        } else {
            ASSERT(false) << "Only straight lines are supported";
        }
        return *this;
    }

} // namespace ui