#include "canvas.h"
#include "root_window.h"

namespace ui {

	Canvas::VisibleRegion::VisibleRegion(RootWindow* root) :
		root(root), 
		windowOffset(0, 0) {
		if (root != nullptr)
		    region = Rect(root->width(), root->height());
	}

	Canvas::VisibleRegion::VisibleRegion(VisibleRegion const& from, Coord left, Coord top, Coord width, Coord height) :
		root(root),
	    region(Rect::Intersection(from.region, Rect(left, top, left + width, top + height))) {
		// if the new visible region is not empty, update the offset, otherwise the offset's value does not matter
		if (!region.empty())
			windowOffset = Point(
				from.windowOffset.col + (region.left - from.region.left),
				from.windowOffset.row + (region.top - from.region.top)
			);
	}

	Canvas::Canvas(Canvas const& from, Coord left, Coord top, Coord width, Coord height) :
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

} // namespace ui