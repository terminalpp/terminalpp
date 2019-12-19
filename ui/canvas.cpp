#include "selection.h"
#include "root_window.h"

#include "canvas.h"


namespace ui {

    // Canvas::VisibleRegion

    Canvas::VisibleRegion::VisibleRegion(RootWindow * root):
        root(root),
        region(Rect::FromWH(root->width(), root->height())),
        windowOffset(0, 0),
        valid{true} {
    }

/*
    Canvas::VisibleRegion::VisibleRegion(VisibleRegion const & from, int left, int top, int width, int height):
        root(from.root),
	    region(Rect::Intersection(from.region, Rect(left, top, left + width, top + height))),
        valid{true} {
		// if the new visible region is not empty, update the intersection by the control start and update the offset, otherwise the offset's value does not matter
		if (!region.empty()) {
			region = region - Point(left, top);
			windowOffset = Point(
				from.windowOffset.x + (left + region.left() - from.region.left()),
				from.windowOffset.y + (top + region.top() - from.region.top())
			);
		}
		ASSERT(windowOffset.x >= 0 && windowOffset.y >= 0);
    } */

    Canvas::VisibleRegion::VisibleRegion(VisibleRegion const & from, Rect subset):
        root(from.root),
	    region(Rect::Intersection(from.region, subset)),
        valid{true} {
		// if the new visible region is not empty, update the intersection by the control start and update the offset, otherwise the offset's value does not matter
		if (!region.empty()) {
			region = region - subset.topLeft();
			windowOffset = Point(
				from.windowOffset.x + (subset.left() + region.left() - from.region.left()),
				from.windowOffset.y + (subset.top() + region.top() - from.region.top())
			);
		}
		ASSERT(windowOffset.x >= 0 && windowOffset.y >= 0);
    }

    // Canvas

/*
    Canvas::Canvas(Canvas & from, int left, int top, int width, int height):
        width_(width),
        height_(height),
        visibleRegion_(from.visibleRegion_, left, top, width, height),
        buffer_(visibleRegion_.root->buffer_) {
        ++buffer_.lockDepth_;
    } */

    Canvas::Canvas(Canvas & from, Rect subset):
        width_(subset.width()),
        height_(subset.height()),
        visibleRegion_(from.visibleRegion_, subset),
        buffer_(visibleRegion_.root->buffer_) {
        ++buffer_.lockDepth_;
    }

    Canvas::Canvas(Canvas const & from):
        width_(from.width_),
        height_(from.height_),
        visibleRegion_(from.visibleRegion_),
        buffer_(from.buffer_) {
        ++buffer_.lockDepth_;
    }

/*    Canvas::Canvas(Canvas && from):
        width_(from.width_),
        height_(from.height_),
        visibleRegion_(from.visibleRegion_),
        buffer_(from.buffer_) {
        ++buffer_.lockDepth_;
    } */

    Canvas::~Canvas() {
        if (--buffer_.lockDepth_ == 0) {
            buffer_.lock_.unlock();
            visibleRegion_.root->render(
                Rect::FromWH(visibleRegion_.region.width(), visibleRegion_.region.height()) + visibleRegion_.windowOffset
            );
        }
    }

    Canvas::Canvas(VisibleRegion const & visibleRegion, int width, int height):
        width_(width),
        height_(height),
        visibleRegion_(visibleRegion),
        buffer_(visibleRegion.root->buffer_) {
        ++buffer_.lockDepth_;
    }

    Canvas Canvas::Create(Widget const & widget) {
        ASSERT(widget.visibleRegion_.valid);
        Buffer & b = widget.visibleRegion_.root->buffer_;
        b.lock_.lock();
        ASSERT(b.lockDepth_ == 0);
        return Canvas{widget.visibleRegion_, widget.width_, widget.height_};
    }

    void Canvas::setCursor(Cursor const & cursor) {
        // set the cursor behavior in root to given cursor info and position 0 0
        visibleRegion_.root->cursor_ = cursor;
        visibleRegion_.root->cursor_.pos = Point(0,0);
        // mark the cell as containing the cursor and update the cursor position with proper coordinates if in range
        Cell * cell = at(cursor.pos);
        if (cell != nullptr) {
            cell->setCursor(true);
            visibleRegion_.root->cursor_.pos = visibleRegion_.translate(cursor.pos);
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

    void Canvas::borderClear(Rect const & rect) {
        for (int y = rect.top(), ye = rect.bottom(); y < ye; ++y)
            for (int x = rect.left(), xe = rect.right(); x < xe; ++x)
                if (Cell * cell = at(Point{x, y}))
                    cell->setAttributes(cell->attributes().setBorderTop(false).setBorderLeft(false).setBorderBottom(false).setBorderRight(false));
    }

    void Canvas::borderRect(Rect const & rect, Color color, bool thick) {
        borderLineTop(rect.topLeft(), rect.width(), color, thick);
        borderLineBottom(Point(rect.topLeft().x, rect.bottomRight().y - 1), rect.width(), color, thick);
        borderLineLeft(rect.topLeft(), rect.height(), color, thick);
        borderLineRight(Point(rect.bottomRight().x - 1, rect.topLeft().y), rect.height(), color, thick);
    }

    void Canvas::borderLineTop(Point start, int width, Color color, bool thick) {
        int end = start.x + width;
        for (; start.x < end; ++start.x) {
            if (Cell * cell = at(start)) 
                cell->setBorder(color).setAttributes(cell->attributes().setBorderTop().setBorderThick(thick));
        }
    }

    void Canvas::borderLineBottom(Point start, int width, Color color, bool thick) {
        int end = start.x + width;
        for (; start.x < end; ++start.x) {
            if (Cell * cell = at(start)) 
                cell->setBorder(color).setAttributes(cell->attributes().setBorderBottom().setBorderThick(thick));
        }
    }

    void Canvas::borderLineLeft(Point start, int height, Color color, bool thick) {
        int end = start.y + height;
        for (; start.y < end; ++start.y) {
            if (Cell * cell = at(start)) 
                cell->setBorder(color).setAttributes(cell->attributes().setBorderLeft().setBorderThick(thick));
        }
    }

    void Canvas::borderLineRight(Point start, int height, Color color, bool thick) {
        int end = start.y + height;
        for (; start.y < end; ++start.y) {
            if (Cell * cell = at(start)) 
                cell->setBorder(color).setAttributes(cell->attributes().setBorderRight().setBorderThick(thick));
        }
    }



} // namespace ui