#include "helpers/time.h"
#include "helpers/log.h"

#include "renderer.h"

#include "widgets/panel.h"

#include "root_window.h"

namespace ui {

	RootWindow::RootWindow():
		Widget{},
		Container{},
		destroying_{false},
		renderer_{nullptr},
		buffer_{0, 0},
        windowFocused_{false},
		keyboardFocus_{this},
		lastMouseTarget_{nullptr},
		mouseFocus_{this},
        mouseIn_{false},
		mouseCaptured_{0},
        mouseClickDuration_{200},
        mouseDoubleClickDuration_{400},
		mouseClickTarget_{nullptr},
		mouseClickButton_{MouseButton::Left}, // does not matter
		mouseClickStart_{0},
		mouseClickEnd_{0},
		pasteRequestTarget_{nullptr},
		selectionOwner_{nullptr},
		title_{""},
		icon_{Icon::Default},
		backgroundColor_{Color::Black},
		modalPane_{new Panel{}},
		modalFocusBackup_{nullptr},
		modalWidgetActive_{false} {
		visibleRect_ = Canvas::VisibleRect{Rect::FromWH(width_, height_), Point{0, 0}, this};
		modalPane_->parent_ = this;
		modalPane_->setBackground(Color::Black.withAlpha(128));
		modalPane_->setLayout(Layout::Maximized);
	}

	RootWindow::~RootWindow() {
		// one-way detach all children
		//for (Widget * child : children())
		//    child->detach
		// first set the destroy flag
		destroying_ = true;
		// then invalidate
		invalidate();
		// obtain the buffer - sync with end of any pending paints
		buffer();
		// deletes the modal pane - the modal pane is never really attached, so we just reset its parent pointer
		modalPane_->parent_ = nullptr;
		delete modalPane_;
	}


	void RootWindow::showModalWidget(Widget * w, Widget * keyboardFocus) {
		ASSERT(keyboardFocus == w || keyboardFocus->isChildOf(w));
		modalPane_->attachChild(w);
		modalWidgetActive_ = true;
		setOverlay(true);
		modalFocusBackup_ = keyboardFocus_;
		focusWidget(keyboardFocus, true);
		repaint();
	}

	void RootWindow::hideModalWidget() {
		if (modalWidgetActive_) {
			modalWidgetActive_ = false;
			ASSERT(modalFocusBackup_ != nullptr);
			focusWidget(modalFocusBackup_, true);
			modalPane_->detachChild(modalPane_->children_.front());
			setOverlay(false);
			repaint();
		}
	}

	void RootWindow::render(Rect const & rect) {
		if (renderer_) 
		    renderer_->requestRender(rect);
	}

    void RootWindow::mouseDown(int col, int row, MouseButton button, Key modifiers) {
        updateMouseState(col, row);
        // capture the mouse, dispatch mouse down event 
        ++mouseCaptured_;
        if (mouseFocus_ == this)
            Container::mouseDown(col, row, button, modifiers);
        else 
            mouseFocus_->mouseDown(col, row, button, modifiers);
        //mouseFocus_->mouseDown(col, row, button, modifiers);
        // deal with clicks and double clicks, by remembering the button & time of the down event
        mouseClickStart_ = helpers::SteadyClockMillis();
        mouseClickButton_ = button;
        // if mouseFocus has changed between the last click and now, it's not a double click we deal with this by setting the last click end to 0
        if (mouseClickTarget_ != mouseFocus_) {
            mouseClickTarget_ = mouseFocus_;
            mouseClickEnd_ = 0;
        }
    }

    void RootWindow::mouseUp(int col, int row, MouseButton button, Key modifiers) {
        // no need to update mouse state, would have been updated properly by the mouseDown at least
        ASSERT(mouseCaptured_ > 0);
        ASSERT(mouseFocus_ != nullptr);
        // just some defensive programming here
        if (mouseFocus_ == nullptr)
            return;
        // determine the widget's coordinates based on the col & row and trigger the appropriate events
        mouseFocus_->windowToWidgetCoordinates(col, row);
        if (mouseFocus_ == this)
            Container::mouseUp(col, row, button, modifiers);
        else 
            mouseFocus_->mouseUp(col, row, button, modifiers);
        //mouseFocus_->mouseUp(col, row, button, modifiers);
        size_t now = helpers::SteadyClockMillis();
        if (button == mouseClickButton_ && (now - mouseClickStart_ <= mouseClickDuration_)) {
            ASSERT(mouseClickTarget_ == mouseFocus_);
            // is it a double click? 
            if (now - mouseClickEnd_ <= mouseDoubleClickDuration_) {
                mouseFocus_->mouseDoubleClick(col, row, button, modifiers);
                mouseClickEnd_ = 0;
            } else {
                mouseFocus_->mouseClick(col, row, button, modifiers);
                mouseClickEnd_ = now;
            }
        } 
        // if at the end of capture we realize the mouse is out, the delayed mouse out events should be triggered now
        if (--mouseCaptured_ == 0 && mouseIn_ == false) 
            inputMouseOut();
    }

    void RootWindow::mouseWheel(int col, int row, int by, Key modifiers) {
        updateMouseState(col, row);
            if (mouseFocus_ == this)
                Container::mouseWheel(col, row, by, modifiers);
            else 
                mouseFocus_->mouseWheel(col, row, by, modifiers);
    }

    void RootWindow::mouseMove(int col, int row, Key modifiers) {
        if (mouseCaptured_ || (col >= 0 && col < width() && row >= 0 && row < height())) {
            updateMouseState(col, row);
            if (mouseFocus_ == this)
                Container::mouseMove(col, row, modifiers);
            else 
                mouseFocus_->mouseMove(col, row, modifiers);
        }
    }

	void RootWindow::updateMouseState(int & col, int & row) {
		if (mouseCaptured_) {
			ASSERT(mouseFocus_ != nullptr);
			// if the mouse was out, but the coordinates now are inside the window, set mouseIn correctly
			if (! mouseIn_ && col >= 0 && col < width() && row >= 0 && row < height())
				mouseIn_ = true;
			// update the coordinates correctly for the target widget
			mouseFocus_->windowToWidgetCoordinates(col, row);
		} else {
			// when mouse is not captured, mouse events are expected to only arrive if coordinates are within the root window
			ASSERT(col >= 0 && row >= 0 && col < width() && row < height());
			// if mouse is not captured, then we must determine proper focus target and recalculate the coordinates
			Widget * w = modalWidgetActive_ ? modalPane_->getMouseTarget(col, row) : getMouseTarget(col, row);
			// if this is first mouse event then we need to signal mouseIn for the root window first and then update the mouse focus target and call its mouseEnter
			if (! mouseIn_) {
				ASSERT(mouseFocus_ == this);
				mouseIn_ = true;
				mouseIn();
				mouseFocus_ = w;
				mouseFocus_->mouseEnter();
			// otherwise, if the calculated target is different from the previous one, we must emit mouse leave and mouse enter events properly
			} else if (w != mouseFocus_) {
				mouseFocus_->mouseLeave();
				mouseFocus_ = w;
				mouseFocus_->mouseEnter();
			}
		}

		// if the mouse is inside the window, do nothing as either we'll get the inputMouseOut signal, or the mouse is in anyways
		if (mouseIn_)
			return;
		// don't do anything if the mouse is outside of the window
		if (col < 0 || row < 0 || col >= width() || row >= height())
			return;
		// otherwise if capture is on, just disable the flag
		if (mouseCaptured_) {
			mouseIn_ = true;
		// if capture is off,
		} else {
			Widget * w = getMouseTarget(col, row);
			ASSERT(w != nullptr);
			ASSERT(w->rootWindow() == this);
		}
	}        


    void RootWindow::keyChar(helpers::Char c) {
		if (keyboardFocus_)
		    keyboardFocus_->keyChar(c);
		else 
            Container::keyChar(c);
    }

    void RootWindow::keyDown(Key k) {
		if (keyboardFocus_)
		    keyboardFocus_->keyDown(k);
		else 
            Container::keyDown(k);
    }

    void RootWindow::keyUp(Key k) {
		if (keyboardFocus_)
		    keyboardFocus_->keyUp(k);
		else 
            Container::keyUp(k);
    }

    void RootWindow::invalidateContents() {
        Container::invalidateContents();
        // if the root window is in the process of being destroyed, don't revalidate
        if (! destroying_)
            visibleRect_ = Canvas::VisibleRect(Rect::FromWH(width_, height_), Point{0,0}, this);
    }

	void RootWindow::updateSize(int width, int height) {
		// resize the buffer first
		{
			std::lock_guard<Canvas::Buffer> g(buffer_);
			buffer_.resize(width, height);
			// we can't simply resize the widget as this would trigger a conflicting repaint because the modal pane is not part of the main tree. This invalidates the pane first without repainting and then changes the size
			modalPane_->visibleRect_.invalidate();
			modalPane_->resize(width, height);
		}
		Container::updateSize(width, height);
	}

	bool RootWindow::focusWidget(Widget * widget, bool value) {
		ASSERT(widget != nullptr);
		if (modalWidgetActive_ && ! widget->isChildOf(modalPane_))
			return false;
		// if the widget is not the root window, update its focus accordingly and set the focused
		if (widget != this) {
			if (focused()) {
				ASSERT(keyboardFocus_ == widget || value);
				if (keyboardFocus_ != nullptr) 
					keyboardFocus_->updateFocused(false);
				keyboardFocus_ = value ? widget : nullptr;
				if (keyboardFocus_ != nullptr)
					keyboardFocus_->updateFocused(true);
			} else {
				keyboardFocus_ = value ? widget : nullptr;
			}
		// if the widget is root window, then just call own updateFocused method, which sets the focus of the root window and updates the focus of the active widget, if any
		} else {
			updateFocused(value);
		}
		return true;
	}

	Widget * RootWindow::focusNext() {
		std::map<unsigned, Widget*>::iterator i = focusStops_.begin();
		if (keyboardFocus_ != nullptr && keyboardFocus_->focusStop()) {
			i = focusStops_.find(keyboardFocus_->focusIndex());
			++i;
			if (i == focusStops_.end())
				i = focusStops_.begin();
		}
		// now we have the iterator to the widget to be focused
		if (modalWidgetActive_) {
			size_t x = 0;
			while (x < focusStops_.size()) {
				if (i->second->isChildOf(modalPane_))
					break;
				++x;
				++i;
				if (i == focusStops_.end())
					i = focusStops_.begin();
			}
			if (x == focusStops_.size())
				return keyboardFocus_;
		}
		if (i != focusStops_.end())
			focusWidget(i->second, true);
		return keyboardFocus_;
	}

	void RootWindow::updateTitle(std::string const & title) {
		title_ = title;
		if (renderer_)
			renderer_->setTitle(title_);
	}

	void RootWindow::updateIcon(Icon icon) {
		icon_ = icon;
		if (renderer_)
			renderer_->setIcon(icon_);
	}

	void RootWindow::closeRenderer() {
		if (renderer_)
		    renderer_->requestClose();
	}

	void RootWindow::paint(Canvas & canvas) {
		Container::paint(canvas);
		if (modalWidgetActive_)
	        paintChild(modalPane_, canvas);
	}

	void RootWindow::requestClipboardContents(Widget * sender) {
		if (pasteRequestTarget_ == nullptr) {
			pasteRequestTarget_ = sender;
			if (renderer_) 
			    renderer_->requestClipboardContents();
			else
			    LOG() << "Paste request w/o renderer";
		} else {
			LOG() << "Paste request clash";
		}
	}

	void RootWindow::requestSelectionContents(Widget * sender) {
		if (pasteRequestTarget_ == nullptr) {
			pasteRequestTarget_ = sender;
			if (renderer_) 
			    renderer_->requestSelectionContents();
			else
			    LOG() << "Paste request w/o renderer";
		} else {
			LOG() << "Paste request clash";
		}
	}

	void RootWindow::paste(std::string const & contents) {
		if (pasteRequestTarget_ != nullptr) {
		    pasteRequestTarget_->paste(contents);
			pasteRequestTarget_ = nullptr;
		} else if (keyboardFocus_ != nullptr) {
			keyboardFocus_->paste(contents);
		} else {
			LOG() << "Paste event received w/o active request or focused widget";
		}
	}

	void RootWindow::setClipboard(std::string const & contents) {
		// clipboard is simple as there is no state associated with it, we simply inform the renderer that clipboard state should be changed
		if (renderer_) 
			renderer_->setClipboard(contents);
		else
			LOG() << "Set clipboard event in an unattached root window";
	}

	void RootWindow::setSelection(Widget * owner, std::string const & contents) {
		if (selectionOwner_ != owner) {
			if (selectionOwner_ != nullptr)
				selectionOwner_->selectionInvalidated();
			selectionOwner_ = owner;
		}
		if (renderer_)
			renderer_->setSelection(contents);
		else
			LOG() << "Set Selection event when no renderer attached";
	}

	void RootWindow::clearSelection() {
		selectionInvalidated();
		if (renderer_)
		    renderer_->clearSelection();
		else
			LOG() << "Clear selection event without renderer";
	}

	void RootWindow::selectionInvalidated() {
		if (selectionOwner_) {
			selectionOwner_->selectionInvalidated();
			selectionOwner_ = nullptr;
		} else {
			LOG() << "invalidate selection w/o selection owner present";
		}
	}




} // namespace ui