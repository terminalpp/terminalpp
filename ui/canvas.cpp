#include "canvas.h"

#include "root_window.h"

namespace ui {

    // Canvas::VisibleRegion

    Canvas::VisibleRegion::VisibleRegion(RootWindow * root):
        root(root),
        region(root->width(), root->height()),
        windowOffset(0, 0) {

    }

    Canvas::VisibleRegion::VisibleRegion(VisibleRegion const & from, int left, int top, int width, int height):
        root(from.root),
	    region(Rect::Intersection(from.region, Rect(left, top, left + width, top + height))) {
		// if the new visible region is not empty, update the intersection by the control start and update the offset, otherwise the offset's value does not matter
		if (!region.empty()) {
			region = region - Point(left, top);
			windowOffset = Point(
				from.windowOffset.x + (left + region.left() - from.region.left()),
				from.windowOffset.y + (top + region.top() - from.region.top())
			);
		}
		ASSERT(windowOffset.x >= 0 && windowOffset.y >= 0);
    }

    // Canvas

    Canvas::Canvas(Canvas & from, int left, int top, int width, int height):
        width_(width),
        height_(height),
        visibleRegion_(from.visibleRegion_, left, top, width, height),
        buffer_(visibleRegion_.root->buffer_) {
        ++buffer_.lockDepth_;
    }

    Canvas::~Canvas() {
        if (--buffer_.lockDepth_ == 0) {
            buffer_.lock_.unlock();
            visibleRegion_.root->repaint(
                Rect(visibleRegion_.region.width(), visibleRegion_.region.height()) + visibleRegion_.windowOffset
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
        ASSERT(widget.visibleRegion_.valid());
        Buffer & b = widget.visibleRegion_.root->buffer_;
        b.lock_.lock();
        ASSERT(b.lockDepth_ == 0);
        return Canvas(widget.visibleRegion_, widget.width_, widget.height_);
    }

    void Canvas::setCursor(Cursor const & cursor) {
        // set the cursor behavior in root to given cursor info and position 0 0
        visibleRegion_.root->cursor_ = cursor;
        visibleRegion_.root->cursor_.pos = Point(0,0);
        // mark the cell as containing the cursor and update the cursor position with proper coordinates if in range
        Cell * cell = at(cursor.pos);
        if (cell != nullptr) {
            *cell << cursor;
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
				if (Cell * c = at(p)) {
                    *c << Background(brush.color.blendOver(c->background()));
                    if (brush.fill == helpers::Char::NUL) {
                        *c << Foreground(brush.color.blendOver(c->foreground()))
                           << DecorationColor(brush.color.blendOver(c->decorationColor()));
                    } else {
                        *c << brush.fill 
                           << Foreground(brush.fillColor)
                           << brush.fillFont; 
                    }
				}
			}
		}
	}

	void Canvas::textOut(Point start, std::string const& text, Color color, Font font) {
		char const* i = text.c_str();
		char const* e = i + text.size();
		while (i < e) {
			if (start.x >= width_) // don't draw past first line
				break;
			helpers::Char const * c = helpers::Char::At(i, e);
			if (Cell * cell = at(start)) 
                *cell << c->codepoint() << Foreground(color) << font;
			++start.x;
		}
	}




} // namespace ui