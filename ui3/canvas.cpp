#include "renderer.h"
#include "widget.h"

#include "canvas.h"

namespace ui3 {

    Canvas::Canvas(VisibleArea const & visibleArea, Size const & size):
        visibleArea_{visibleArea},
        buffer_{visibleArea.renderer()->buffer_}, 
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


    Canvas & Canvas::fill(Rect const & rect, Color color) {
        Rect r = (rect & visibleArea_.rect()) + visibleArea_.offset();
        if (color.opaque()) {
            for (int y = r.top(), ye = r.bottom(); y < ye; ++y) {
                for (int x = r.left(), xe = r.right(); x < xe; ++x) {
                    Cell & c = buffer_.at(x,y);
                    c.bg() = color;
                    c.codepoint() = ' ';
                    // TODO border
                }
            }
        } else {
            for (int y = r.top(), ye = r.bottom(); y < ye; ++y) {
                for (int x = r.left(), xe = r.right(); x < xe; ++x) {
                    Cell & c = buffer_.at(x,y);
                    c.fg() = color.blendOver(c.fg());
                    c.bg() = color.blendOver(c.bg());
                    c.decor() = color.blendOver(c.decor());
                    // TODO border
                }
            }
        }
        return *this;
    }

    Canvas & Canvas::textOut(Point x, Char::iterator_utf8 begin, Char::iterator_utf8 end) {
        Rect vr = visibleArea_.rect() + visibleArea_.offset();
        x = x + visibleArea_.offset();
        for (; begin != end; ++begin) {
            if (vr.contains(x)) {
                Cell & c = buffer_.at(x);
                c.fg() = fg_;
                c.decor() = decor_;
                c.bg() = bg_.blendOver(c.bg());
                c.font() = font_;
                c.codepoint() = begin->codepoint();
            }
            x.setX(x.x() + Char::ColumnWidth(*begin) * font_.width());
        }
        return *this;
    }


} // namespace ui