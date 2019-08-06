#include "widget.h"
#include "root_window.h"

namespace ui {


	
	void Widget::setFocus(bool value) {
		if (focused_ != value && visibleRegion_.valid())
			visibleRegion_.root->focusWidget(this); 
	}


	void Widget::repaint() {
		// only repaint the control if it is visible
		if (visible_ && visibleRegion_.valid()) {
			// if the widget is overlaid, the parent must be repainted, if the parent is not known, it is likely a root window, so just repainting it is ok
			if (parent_ != nullptr && (forceOverlay_ || overlay_)) {
				parent_->repaint();
			// otherwise repainting the widget is enough
			} else {
				Canvas canvas = Canvas::Create(*this);
				paint(canvas);
			}
		} 
	}

	void Widget::paintChild(Widget * child, Canvas& clientCanvas) {
		if (!child->visible_)
			return;
		if (!child->visibleRegion_.valid()) {
			Canvas childCanvas(clientCanvas, child->x_, child->y_, child->width_, child->height_);
			child->visibleRegion_ = childCanvas.visibleRegion_;
			child->paint(childCanvas);
		} else {
			Canvas childCanvas(child->visibleRegion_, child->width_, child->height_);
			child->paint(childCanvas);
		}
	}

} // namespace ui