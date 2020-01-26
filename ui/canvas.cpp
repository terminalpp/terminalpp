#include "traits/selection.h"
#include "root_window.h"

#include "widget.h"
#include "canvas.h"

namespace ui {

    // Canvas

    Canvas::Canvas(Widget const * widget):
        width_(widget->width()),
        height_(widget->height()),
        visibleRect_(widget->visibleRect_),
        // TODO the locking should be less error prone and ad hoc
        buffer_(widget->visibleRect_.rootWindow_->buffer_) {
        buffer_.lock_.lock();
        ++buffer_.lockDepth_;
    }

    Canvas::Canvas(Canvas const & from):
        width_(from.width_),
        height_(from.height_),
        visibleRect_(from.visibleRect_),
        buffer_(from.buffer_) {
        ++buffer_.lockDepth_;
    }
    
    Canvas & Canvas::clip(Rect const & rect) {
        // first update the visible rectangle
        Rect vr{Rect::Intersection(visibleRect_.rect_, rect)};
        visibleRect_.bufferOffset_ += Point{vr.left() - visibleRect_.rect_.left(), vr.top() - visibleRect_.rect_.top()};
        visibleRect_.rect_ = vr - rect.topLeft();
        // then update width and height
        width_ = rect.width();
        height_ = rect.height();
        return *this;
    }

    Canvas & Canvas::resize(int width, int height) {
        ASSERT(width >= 0 && height >= 0);
        width_ = width;
        height_ = height;
        visibleRect_.rect_ = Rect::Intersection(visibleRect_.rect_, Rect::FromWH(width_, height_));
        return *this;
    }

    Canvas & Canvas::scrollBy(Point const & offset) {
        visibleRect_.rect_ += offset;
        Rect vr{Rect::Intersection(visibleRect_.rect_, Rect::FromWH(width_, height_))};
        visibleRect_.bufferOffset_ += Point{vr.left() - visibleRect_.rect_.left(), vr.top() - visibleRect_.rect_.top()};
        visibleRect_.rect_ = vr;
        return *this;
    }


    Canvas::~Canvas() {
        if (--buffer_.lockDepth_ == 0) {
            buffer_.lock_.unlock();
            visibleRect_.rootWindow_->render(Rect::FromTopLeftWH(visibleRect_.bufferOffset_, visibleRect_.rect_.width(), visibleRect_.rect_.height()));
        }
    }

    void Canvas::setCursor(Cursor const & cursor) {
        // set the cursor behavior in root to given cursor info and position 0 0
        visibleRect_.rootWindow_->cursor_ = cursor;
        visibleRect_.rootWindow_->cursor_.pos = Point(0,0);
        // mark the cell as containing the cursor and update the cursor position with proper coordinates if in range
        Cell * cell = at(cursor.pos);
        if (cell != nullptr) {
            cell->setCursor(true);
            visibleRect_.rootWindow_->cursor_.pos = cursor.pos - visibleRect_.rect_.topLeft() + visibleRect_.bufferOffset_;;
        }
    }

	void Canvas::fill(Rect const& rect, Brush const& brush) {
		// don't do anything if the brush is empty
		if (brush == Brush::None())
			return;
		// otherwise apply the brush to the cells
		Point p;
		for (p.y = rect.top(); p.y < rect.bottom(); ++p.y) {
			for (p.x = rect.left(); p.x < rect.right(); ++p.x) {
                fill(at(p), brush);
			}
		}
	}

    void Canvas::fill(Selection const & sel, Brush const & brush) {
		// don't do anything if the brush is empty
		if (brush == Brush::None())
			return;
        if (sel.start().y + 1 == sel.end().y) {
            Point p = sel.start();
            for (; p.x < sel.end().x; ++p.x)
                fill(at(p), brush);
        } else {
            Point p = sel.start();
            for (; p.x < width_; ++p.x)
                fill(at(p), brush);
            for (p.y = sel.start().y + 1; p.y < sel.end().y - 1; ++p.y)
                for (p.x = 0; p.x < width_; ++p.x)
                    fill(at(p), brush);
            for (p.y = sel.end().y - 1, p.x = 0; p.x < sel.end().x; ++p.x)
                fill(at(p), brush);
        }
    }

    void Canvas::fill(Cell * cell, Brush const & brush) {
        if (!cell)
            return;
        cell->setBg(brush.color.blendOver(cell->bg()));
        if (brush.fill == helpers::Char::NUL) {
            cell->setFg(brush.color.blendOver(cell->fg()))
                 .setDecoration(brush.color.blendOver(cell->decoration()));
        } else {
            cell->setCodepoint(brush.fill) 
                 .setFg(brush.fillColor)
                 .setFont(brush.fillFont); 
        }

    }

	void Canvas::textOut(Point start, std::string const& text, Color color, Font font) {
		char const* i = text.c_str();
		char const* e = i + text.size();
        int fontWidth = font.width();
		while (i < e) {
			if (start.x >= width_) // don't draw past first line
				break;
            helpers::Char const * c = helpers::Char::At(i, e);
            for (int j = 0; j < fontWidth; ++j) {
                if (Cell * cell = at(start)) 
                    cell->setCodepoint(c->codepoint()).setFg(color).setFont(font);
                ++start.x;
            }
		}
	}

    void Canvas::clearBorder(Rect const & rect) {
        for (int y = rect.top(), ye = rect.bottom(); y < ye; ++y)
            for (int x = rect.left(), xe = rect.right(); x < xe; ++x)
                if (Cell * cell = at(Point{x, y}))
                    cell->setAttributes(cell->attributes().setBorderTop(false).setBorderLeft(false).setBorderBottom(false).setBorderRight(false));
    }

    void Canvas::borderRect(Rect const & rect, Border const & border) {
        borderLineTop(rect.topLeft(), rect.width(), border.color, border.top);
        borderLineBottom(Point(rect.topLeft().x, rect.bottomRight().y - 1), rect.width(), border.color, border.bottom);
        borderLineLeft(rect.topLeft(), rect.height(), border.color, border.left);
        borderLineRight(Point(rect.bottomRight().x - 1, rect.topLeft().y), rect.height(), border.color, border.right);
    }

    void Canvas::borderLineTop(Point start, int width, Color color, Border::Kind kind) {
        if (kind == Border::Kind::None)
            return;
        int end = start.x + width;
        for (; start.x < end; ++start.x) {
            if (Cell * cell = at(start)) 
                cell->setBorder(color).setAttributes(cell->attributes().setBorderTop().setBorderThick(kind == Border::Kind::Thick));
        }
    }

    void Canvas::borderLineBottom(Point start, int width, Color color, Border::Kind kind) {
        if (kind == Border::Kind::None)
            return;
        int end = start.x + width;
        for (; start.x < end; ++start.x) {
            if (Cell * cell = at(start)) 
                cell->setBorder(color).setAttributes(cell->attributes().setBorderBottom().setBorderThick(kind == Border::Kind::Thick));
        }
    }

    void Canvas::borderLineLeft(Point start, int height, Color color, Border::Kind kind) {
        if (kind == Border::Kind::None)
            return;
        int end = start.y + height;
        for (; start.y < end; ++start.y) {
            if (Cell * cell = at(start)) 
                cell->setBorder(color).setAttributes(cell->attributes().setBorderLeft().setBorderThick(kind == Border::Kind::Thick));
        }
    }

    void Canvas::borderLineRight(Point start, int height, Color color, Border::Kind kind) {
        if (kind == Border::Kind::None)
            return;
        int end = start.y + height;
        for (; start.y < end; ++start.y) {
            if (Cell * cell = at(start)) 
                cell->setBorder(color).setAttributes(cell->attributes().setBorderRight().setBorderThick(kind == Border::Kind::Thick));
        }
    }

   	void Canvas::drawRightVerticalScrollBar(Point from, int size,  int sliderStart, int sliderSize, Color color, bool thick) {
        clearBorder(Rect::FromCorners(from.x, 0, from.x + 1, size));
        borderLineRight(Point{from.x, 0}, size, color, thick ? Border::Kind::Thick : Border::Kind::Thin);
        borderLineRight(Point{from.x, sliderStart}, sliderSize, color, Border::Kind::Thick);
        if (thick)
            borderLineLeft(Point{from.x, sliderStart}, sliderSize, color, Border::Kind::Thick);
   }

} // namespace ui