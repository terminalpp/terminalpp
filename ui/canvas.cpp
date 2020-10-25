#include "renderer.h"
#include "widget.h"

#include "canvas.h"

namespace ui {

    Canvas::Canvas(Buffer & buffer, VisibleArea const & visibleArea, Size const & size):
        visibleArea_{visibleArea},
        buffer_{& buffer}, 
        size_{size} {
    }

    std::vector<Canvas::TextLine> Canvas::GetTextMetrics(std::string const & text, int wordWrapAt) {
        std::vector<TextLine> result;
        Char::iterator_utf8 i = Char::BeginOf(text);
        Char::iterator_utf8 e = Char::EndOf(text);
        while (i != e)
            result.push_back(GetTextLine(i, e, wordWrapAt));
        return result;
    }

    Canvas::TextLine Canvas::GetTextLine(Char::iterator_utf8 & begin, Char::iterator_utf8 const & end, int wordWrapAt) {
        TextLine l{0,0, begin, begin};
        while (wordWrapAt == NoWordWrap || l.width < wordWrapAt) {
            if (begin == end) {
                l.end = begin;
                return l;
            } else if (Char::IsLineEnd(*begin)) {
                l.end = begin;
                ++begin;
                return l;
            }
            l.width += Char::ColumnWidth(*begin);
            ++l.chars;
            ++begin;
        }
        // word wrap is enabled and the line is longer, i.e. backtrack to first word separator
        l.end = begin;
        while (l.end != l.begin) {
            --l.chars;
            l.width -= Char::ColumnWidth(*l.end);
            if (Char::IsWordSeparator(*l.end)) {
                begin = l.end;
                ++begin; // move *after* the word separator on the next line
                return l;
            }
            --l.end;
        }
        // there are no words in the line, just break at the word wrap limit mid-word
        return l;
    }

    Canvas & Canvas::drawBuffer(Buffer const & buffer, Point at) {
        // calculate the target rectangle in the canvas and its intersection with the visible rectangle and offset it to the backing buffer coordinates
        Rect r = (Rect{at, buffer.size()} & visibleArea_.rect()) + visibleArea_.offset();
        // calculate the buffer offset for the input buffer
        Point bufferOffset = at + visibleArea_.offset();
        for (int row = r.top(), re = r.bottom(); row < re; ++row) {
            for (int col = r.left(), ce = r.right(); col < ce; ++col) {
                buffer_->at(col, row) = buffer.at(col - bufferOffset.x(), row - bufferOffset.y());
            }
        }
        return *this;
    }

    Canvas & Canvas::fill(Rect const & rect, Color color) {
        Rect r = (rect & visibleArea_.rect()) + visibleArea_.offset();
        if (color.opaque()) {
            for (int y = r.top(), ye = r.bottom(); y < ye; ++y) {
                for (int x = r.left(), xe = r.right(); x < xe; ++x) {
                    Cell & c = buffer_->at(x,y);
                    c.setBg(color);
                    c.setCodepoint(' ');
                    c.setBorder(c.border().clear());
                }
            }
        } else {
            for (int y = r.top(), ye = r.bottom(); y < ye; ++y) {
                for (int x = r.left(), xe = r.right(); x < xe; ++x) {
                    Cell & c = buffer_->at(x,y);
                    c.setFg(color.blendOver(c.fg()));
                    c.setBg(color.blendOver(c.bg()));
                    c.setDecor(color.blendOver(c.decor()));
                    c.border().setColor(color.blendOver(c.border().color()));
                }
            }
        }
        return *this;
    }

    Canvas & Canvas::fill(Rect const & rect, Cell const & fill) {
        Rect r = (rect & visibleArea_.rect()) + visibleArea_.offset();
        for (int y = r.top(), ye = r.bottom(); y < ye; ++y) {
            for (int x = r.left(), xe = r.right(); x < xe; ++x) {
                buffer_->at(x,y) = fill;
            }
        }
        return *this;

    }

    Canvas & Canvas::textOut(Point x, Char::iterator_utf8 begin, Char::iterator_utf8 end) {
        Rect vr = visibleArea_.rect() + visibleArea_.offset();
        x = x + visibleArea_.offset();
        for (; begin != end; ++begin) {
            if (vr.contains(x)) {
                Cell & c = buffer_->at(x);
                c.setFg(fg_);
                c.setDecor(decor_);
                c.setBg(bg_.blendOver(c.bg()));
                c.setFont(font_);
                c.setCodepoint(begin->codepoint());
            }
            x.setX(x.x() + Char::ColumnWidth(*begin) * font_.width());
        }
        return *this;
    }

    Canvas & Canvas::setBorder(Point at, Border const & border) {
        if (border.empty())
            return *this;
        Rect vr = visibleArea_.rect() + visibleArea_.offset();
        at = at + visibleArea_.offset();
        if (vr.contains(at)) 
            buffer_->at(at).setBorder(border);
        return *this;
    }


    Canvas & Canvas::setBorder(Point from, Point to, Border const & border) {
        if (border.empty())
            return *this;
        Rect vr = visibleArea_.rect() + visibleArea_.offset();
        from += visibleArea_.offset();
        to += visibleArea_.offset();
        if (from.x() == to.x()) {
            int inc = (from.y() < to.y()) ? 1 : -1;
            for (; from != to; from.setY(from.y() + inc)) {
                if (vr.contains(from)) {
                    Cell & c = buffer_->at(from);
                    c.setBorder(c.border() + border);
                }
            }
        } else if (from.y() == to.y()) {
            int inc = (from.x() < to.x()) ? 1 : -1;
            for (; from != to; from.setX(from.x() + inc)) {
                if (vr.contains(from)) {
                    Cell & c = buffer_->at(from);
                    c.setBorder(c.border() + border);
                }
            }
        } else {
            ASSERT(false) << "Only straight lines are supported";
        }
        return *this;
    }

    Canvas & Canvas::setBorder(Rect const & rect, Border const & border) {
        this->setBorder(rect.topLeft(), rect.topRight(), Border::Empty(border.color()).setTop(border.top()));
        this->setBorder(rect.bottomLeft() - Point{0,1}, rect.bottomRight() - Point{0,1}, Border::Empty(border.color()).setBottom(border.bottom()));
        this->setBorder(rect.topLeft(), rect.bottomLeft(), Border::Empty(border.color()).setLeft(border.left()));
        this->setBorder(rect.topRight() - Point{1,0}, rect.bottomRight() - Point{1,0}, Border::Empty(border.color()).setRight(border.right()));
        return *this;
    }

    Canvas & Canvas::verticalScrollbar(int size, int offset) {
        if (size > height()) {
            std::pair<int, int> dim = ScrollBarDimensions(height(), size, offset);
            Border b = Border::Empty(Color::Gray.withAlpha(128)).setRight(Border::Kind::Thin);
            int x = width() - 1;
            setBorder(Point{x, 0}, Point{x, dim.first}, b);
            setBorder(Point{x, dim.second}, Point{x, height()}, b);
            b.setRight(Border::Kind::Thick);
            setBorder(Point{x, dim.first}, Point{x, dim.second}, b);
        }
        return *this;
    }

    Canvas & Canvas::horizontalScrollbar(int size, int offset) {
        if (size > width()) {
            std::pair<int, int> dim = ScrollBarDimensions(width(), size, offset);
            Border b = Border::Empty(Color::White.withAlpha(64)).setRight(Border::Kind::Thin);
            int y = height() - 1;
            setBorder(Point{0,y}, Point{dim.first, y}, b);
            setBorder(Point{dim.second, y}, Point{height(), y}, b);
            b.setRight(Border::Kind::Thick);
            setBorder(Point{dim.first, y}, Point{dim.second, y}, b);
        }
        return *this;
    }



} // namespace ui