#include "canvas.h"
#include "root_window.h"

namespace ui {

	Canvas::VisibleRegion::VisibleRegion(RootWindow* root) :
		root(root), 
		windowOffset(0, 0) {
		if (root != nullptr)
		    region = Rect(root->width(), root->height());
	}



	Canvas::VisibleRegion::VisibleRegion(VisibleRegion const& from, int left, int top, int width, int height) :
		root(from.root),
	    region(Rect::Intersection(from.region, Rect(left, top, left + width, top + height))) {
		// if the new visible region is not empty, update the intersection by the control start and update the offset, otherwise the offset's value does not matter
		if (!region.empty()) {
			region = region - Point(left, top);
			windowOffset = Point(
				from.windowOffset.col + (left + region.left - from.region.left),
				from.windowOffset.row + (top + region.top - from.region.top)
			);
		}
		ASSERT(windowOffset.col >= 0 && windowOffset.row >= 0);
	}

	Canvas::Canvas(Canvas const& from, int left, int top, int width, int height) :
		visibleRegion_(from.visibleRegion_, left, top, width, height),
		screen_(from.screen_),
		width_(width),
		height_(height) {
	}

	void Canvas::fill(Rect const& rect, Brush const& brush) {
		// don't do anything if the brush is empty
		if (brush == Brush::None())
			return;
		// otherwise apply the brush to the cells
		Point p;
		for (p.row = rect.top; p.row < rect.bottom; ++p.row) {
			for (p.col = rect.left; p.col < rect.right; ++p.col) {
				if (Cell * c = at(p)) {
					// update the backrgound color of the cell 
					c->setBg(brush.color.blendOver(c->bg()));
					// if there is no fill character, overlay the text color as well
					if (brush.fill == Char::NUL) {
						c->setFg(brush.color.blendOver(c->fg()));
					} else {
						c->setC(brush.fill);
						c->setFg(brush.fillColor);
						c->setFont(brush.fillFont);
					}
				}
			}
		}
	}

	void Canvas::textOut(Point start, std::string const& text) {
		char const* i = text.c_str();
		char const* e = i + text.size();
		while (i < e) {
			if (start.col >= width_) // don't draw past first line
				break;
			Char const* c = Char::At(i, e);
			Cell* cell = at(start);
			if (cell != nullptr)
				cell->setC(*c);
			++start.col;
		}
	}

} // namespace ui