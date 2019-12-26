#include "widget.h"
#include "container.h"
#include "root_window.h"

namespace ui {

    Widget::~Widget() {
        // detach from parent if not detached alerady
        if (parent_) 
            parent_->detachChild(this);
    }

	bool Widget::isChildOf(Widget const * parent) const {
		Widget const * p = this->parent_;
		while (p != nullptr) {
			if (p == parent)
				return true;
			p = p->parent_;
		}
		return false;
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
		if (focused_ != value && visibleRect_.valid())
			visibleRect_.rootWindow()->focusWidget(this, value); 
	}

	void Widget::repaint() {
		// only repaint the control if it is visible
		if (visible_ && visibleRect_.valid()) {
			// if the widget is overlaid, the parent must be repainted, if the parent is not known, it is likely a root window, so just repainting it is ok
			if (parent_ != nullptr && (forceOverlay_ || overlay_)) {
				parent_->repaint();
			// otherwise repainting the widget is enough
			} else {
				// TODO this is an issue, if the widget has been invalidated before we get the canvas, the canvas construction would fail
				Canvas canvas{this};
                // now that the canvas was created, make sure that we are still valid under the lock, otherwise ignore the paint request
                if (visibleRect_.valid())
				    paint(canvas);
			}
		} 
	}

	void Widget::invalidate() {
		if (visibleRect_.valid()) {
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

    void Widget::windowToWidgetCoordinates(int & col, int & row) {
        ASSERT(parent_ != nullptr) << "Not expected to be called un detached widget.";
        // adjust for the offset of the widget inside its parent widget
        col -= x_;
        row -= y_;
        parent_->windowToWidgetCoordinates(col, row);
    }

	void Widget::detachRootWindow() {
		ASSERT(visibleRect_.rootWindow() != nullptr);
		visibleRect_.rootWindow()->widgetDetached(this);
		visibleRect_.detach();
		//visibleRect_.root = nullptr;
	}

} // namespace ui