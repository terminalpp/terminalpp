#include "widget.h"
#include "container.h"
#include "root_window.h"

namespace ui {

    Widget::PaintLockGuard::PaintLockGuard(Widget * w) {
        if (w->rootWindow())
            guard_ = w->rootWindow()->buffer();        
    }

    Widget::~Widget() {
        // detach from parent if not detached alerady
        if (parent_) 
            parent_->detachChild(this);
    }

	void Widget::setVisible(bool value) {
		if (visible_ != value) {
			updateVisible(value);
			if (parent_ != nullptr)
				parent_->childInvalidated(this);
		}
	}

	void Widget::setEnabled(bool value) {
		if (enabled_ != value) {
			updateEnabled(value);
			if (parent_ != nullptr)
				parent_->childInvalidated(this);
		}
	}

	
	void Widget::setFocused(bool value) {
		if (focused_ != value && visibleRegion_.valid)
			visibleRegion_.root->focusWidget(this, value); 
	}

	void Widget::repaint() {
		// only repaint the control if it is visible
		if (visible_ && visibleRegion_.valid) {
			// if the widget is overlaid, the parent must be repainted, if the parent is not known, it is likely a root window, so just repainting it is ok
			if (parent_ != nullptr && (forceOverlay_ || overlay_)) {
				parent_->repaint();
			// otherwise repainting the widget is enough
			} else {
				Canvas canvas = Canvas::Create(*this);
                // now that the canvas was created, make sure that we are still valid under the lock, otherwise ignore the paint request
                if (visibleRegion_.valid)
				    paint(canvas);
			}
		} 
	}

	void Widget::paintChild(Widget * child, Canvas& clientCanvas) {
		if (!child->visible_)
			return;
		if (!child->visibleRegion_.valid) {
			Canvas childCanvas(clientCanvas, child->rect());
			child->visibleRegion_ = childCanvas.visibleRegion_;
			child->paint(childCanvas);
		} else {
			Canvas childCanvas(child->visibleRegion_, child->width_, child->height_);
			child->paint(childCanvas);
		}
	}

	void Widget::invalidate() {
		if (visibleRegion_.valid) {
			invalidateContents();
			if (parent_ != nullptr)
				parent_->childInvalidated(this);
		}
	}



	void Widget::setWidthHint(Layout::SizeHint value) {
		if (widthHint_ != value) {
			widthHint_ = value;
			if (parent_ != nullptr)
				parent_->childInvalidated(this);
		}
	}

	void Widget::setHeightHint(Layout::SizeHint value) {
		if (heightHint_ != value) {
			heightHint_ = value;
			if (parent_ != nullptr)
				parent_->childInvalidated(this);
		}
	}


	void Widget::updateFocusStop(bool value) {
		focusStop_ = value;
		RootWindow * root = rootWindow();
		if (root != nullptr) 
		    focusStop_ ? root->addFocusStop(this) : root->removeFocusStop(this);
	}

	void Widget::updateFocusIndex(unsigned value) {
		RootWindow * root = rootWindow();
		if (focusStop_ && root)
		    root->removeFocusStop(this);
		focusIndex_ = value;
		if (focusStop_ && root)
		    root->addFocusStop(this);
	}

    void Widget::requestClipboardContents() {
        RootWindow * root = rootWindow();
        if (root)
            root->requestClipboardContents(this);
    }

    void Widget::requestSelectionContents() {
        RootWindow * root = rootWindow();
        if (root)
            root->requestSelectionContents(this);
    }

	void Widget::setClipboardContents(std::string const & value) {
        RootWindow * root = rootWindow();
		ASSERT(root != nullptr);
		if (root != nullptr)
		    root->setClipboard(value);
	}

	void Widget::setSelectionContents(std::string const & value) {
        RootWindow * root = rootWindow();
		ASSERT(root != nullptr);
		if (root != nullptr)
		    root->setSelection(this, value);
	}



	void Widget::detachRootWindow() {
		ASSERT(visibleRegion_.root != nullptr);
		visibleRegion_.root->widgetDetached(this);
		visibleRegion_.root = nullptr;
	}

	Point Widget::getMouseCoordinates() const {
		RootWindow * root = rootWindow();
		if (root != nullptr) {
			Point result = root->mouseCoords_;
			root->screenToWidgetCoordinates(this, result.x, result.y);
			return result;
		} else {
			return Point{-1,-1};
		}

	}


} // namespace ui