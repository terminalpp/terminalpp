#include "helpers/time.h"
#include "helpers/log.h"

#include "renderer.h"

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
		backgroundColor_{Color::Black} {
		visibleRect_ = Canvas::VisibleRect{Rect::FromWH(width_, height_), Point{0, 0}, this};
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