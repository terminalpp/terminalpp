#include "layout.h"

#include "container.h"

namespace ui {

	Container::Container(int x, int y, int width, int height) :
		Widget(x, y, width, height),
		layout_(Layout::None()),
		relayout_(true) {
	}

	Container::~Container() {
		for (Widget* child : children_)
			delete child;
	}

	void Container::paint(Canvas& canvas) {
		canvas.fill(Rect(canvas.width(), canvas.height()), Color::Red(), Color::White(), 'x', Font());
		if (relayout_) {
			layout_->relayout(this);
			if (!overlay_)
				layout_->calculateOverlay(this);
			relayout_ = false;
		}
		// display the children
		for (Widget* child : children_)
			paintChild(child, canvas);
	}

	void Container::updateOverlay(bool value) {
		Widget::updateOverlay(value);
		// if the container itself is overlaid, then all its children are overlaid too as they may be part of the parent's overlay
		if (value) {
			for (Widget* child : children_)
				child->updateOverlay(true);
		// otherwise the overlay of the 
		} else {

		}
		// make sure that next repaint will relayout all children, which also sets their overlay flags
		relayout_ = true;

	}

} // namespace ui