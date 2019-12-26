#include "layout.h"

#include "container.h"

namespace ui {

	Container::Container() :
		layout_(Layout::None),
		relayout_(true) {
	}

	void Container::paint(Canvas& canvas) {
		Canvas clientCanvas{getChildrenCanvas(canvas)};
		if (relayout_) {
			layout_->relayout(this, clientCanvas);
			layout_->calculateOverlay(this, overlay_);
			relayout_ = false;
		}
		// display the children
		for (Widget* child : children())
			paintChild(child, clientCanvas);
	}

	void Container::paintChild(Widget * child, Canvas & clientCanvas) {
		if (!child->visible_)
		    return;
		// get the child canvas and don't bother drawing if the canvas has empty visible region
		Canvas childCanvas{clientCanvas};
		childCanvas.clip(child->rect());
		//Canvas childCanvas(clientCanvas, child->rect());
		if (childCanvas.visibleRect_.empty())
			return;
		// otherwise update the child's visible rectangle
		child->visibleRect_ = childCanvas.visibleRect_;
		// finally, paint the child
		child->paint(childCanvas);
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