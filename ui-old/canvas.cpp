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

    Canvas & Canvas::drawBuffer(ScreenBuffer const & buffer, Point topLeft) {
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
                buffer_.at(col, row) = state_;
            }
        }
        return *this;
    }

    Canvas & Canvas::fillRect(Rect const & rect, Color background) {
        Rect r = (rect & visibleRect_) + bufferOffset_;
        for (int row = r.top(), re = r.bottom(); row < re; ++row) {
            for (int col = r.left(), ce = r.right(); col < ce; ++col) {
                Cell & c = buffer_.at(col, row);
                c.setFg(background.blendOver(c.fg()))
                 .setBg(background.blendOver(c.bg()))
                 .setDecor(background.blendOver(c.decor()))
                 .setBorder(c.border().setColor(background.blendOver(c.border().color())));
            }
        }
        return *this;
    }

    Canvas & Canvas::drawBorderLine(Border const & border, Point from, Point to) {
        if (border.empty())
            return *this;
        if (from.x() == to.x()) {
            int inc = (from.y() < to.y()) ? 1 : -1;
            for (; from != to; from.setY(from.y() + inc)) {
                updateBorder(from, border);
            }
        } else if (from.y() == to.y()) {
            int inc = (from.x() < to.x()) ? 1 : -1;
            for (; from != to; from.setX(from.x() + inc)) {
                updateBorder(from, border);
            }
        } else {
            ASSERT(false) << "Only straight lines are supported";
        }
        return *this;
    }

    Canvas & Canvas::drawBorderRect(Border const & border, Rect const & rect) {
        drawBorderLine(Border{border.color()}.setTop(border.top()), rect.topLeft(), rect.topRight());
        drawBorderLine(Border{border.color()}.setBottom(border.bottom()), rect.bottomLeft() - Point{0,1}, rect.bottomRight() - Point{0,1});
        drawBorderLine(Border{border.color()}.setLeft(border.left()), rect.topLeft(), rect.bottomLeft());
        drawBorderLine(Border{border.color()}.setRight(border.right()), rect.topRight() - Point{1,0}, rect.bottomRight() - Point{1,0});
        return *this;
    }

    Canvas & Canvas::textOut(Point where, Char::iterator_utf8 begin, Char::iterator_utf8 end) {
        if (where.y() < visibleRect_.top() || where.y() >= visibleRect_.bottom())
            return *this;
        for (; begin < end; ++begin) {
            if (where.x() >= visibleRect_.right())
                break;
            if (Cell * cell = at(where)) {
                (*cell).setCodepoint((*begin).codepoint())
                       .setFg(state_.fg())
                       .setDecor(state_.decor())
                       .setFont(state_.font());
            } 
            where.setX(where.x() + state_.font().width());
        } 
        return *this;
    }

    Canvas & Canvas::textOut(Point where, StringLine const & line, int maxWidth, HorizontalAlign hAlign) {
        switch (hAlign) {
            case HorizontalAlign::Left:
                break;
            case HorizontalAlign::Center:
                where += Point{(maxWidth - line.width) / 2, 0};
                break;
            case HorizontalAlign::Right:
                where += Point{maxWidth - line.width, 0};
                break;
            default:
                UNREACHABLE;
                break;
        }
        return textOut(where, line.begin, line.end);
    }


} // namespace ui