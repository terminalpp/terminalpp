#include "canvas.h"
#include "root_window.h"

namespace ui {

	Canvas::VisibleRegion::VisibleRegion(RootWindow* root) :
		root(root), 
		windowOffset(0, 0) {
		if (root != nullptr)
		    region = Rect(root->width(), root->height());
	}



	Canvas::VisibleRegion::VisibleRegion(VisibleRegion const& from, int left, int top, unsigned width, unsigned height) :
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
	}



	Canvas::Canvas(Canvas const& from, int left, int top, unsigned width, unsigned height) :
		visibleRegion_(from.visibleRegion_, left, top, width, height),
		screen_(from.screen_),
		width_(width),
		height_(height) {
	}

	void Canvas::fill(Rect rect, Color bg, Color fg, Char fill, Font font) {
		Point p;
		for (p.row = rect.top; p.row < rect.bottom; ++p.row)
			for (p.col = rect.left; p.col < rect.right; ++p.col) {
				Cell* c = at(p);
				if (c != nullptr)
					*c = Cell(fill, fg, bg, font);
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