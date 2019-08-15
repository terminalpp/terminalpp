#include "layout.h"

#include "container.h"

namespace ui {

	Container::Container() :
		layout_(Layout::None()),
		relayout_(true) {
	}

	void Container::paint(Canvas& canvas) {
		Canvas clientCanvas = getClientCanvas(canvas);
		if (relayout_) {
			layout_->relayout(this, clientCanvas);
			if (!overlay_)
				layout_->calculateOverlay(this);
			relayout_ = false;
		}
		// display the children
		for (Widget* child : children())
			paintChild(child, clientCanvas);
	}

	void Container::updateOverlay(bool value) {
		Widget::updateOverlay(value);
		// if the container itself is overlaid, then all its children are overlaid too as they may be part of the parent's overlay
		if (value) {
			for (Widget* child : children())
				child->updateOverlay(true);
		} 
		// make sure that next repaint will relayout all children, which also sets their overlay flags
		relayout_ = true;

	}

} // namespace ui