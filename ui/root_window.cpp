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
		keyboardFocus_{nullptr},
		lastMouseTarget_{nullptr},
		mouseFocus_{nullptr},
		mouseFocusLock_{0},
		mouseCoords_{-1,-1},
		mouseClickWidget_{nullptr},
		mouseClickButton_{MouseButton::Left}, // does not matter
		mouseClickStart_{0},
		mouseDoubleClickPrevious_{0},
		pasteRequestTarget_{nullptr},
		selectionOwner_{nullptr},
		title_{""},
		icon_{Icon::Default},
		background_{Color::Black()} {
		visibleRegion_ = Canvas::VisibleRegion{this};
	}

	void RootWindow::render(Rect const & rect) {
		if (renderer_) 
		    renderer_->requestRender(rect);
	}

    void RootWindow::mouseDown(int col, int row, MouseButton button, Key modifiers) {
		mouseCoords_ = Point{col, row};
		if (++mouseFocusLock_ > 1) {
			if (mouseFocus_ != nullptr) {
				screenToWidgetCoordinates(mouseFocus_, col, row);
				mouseFocus_->mouseDown(col, row, button, modifiers);
			}
		} else {
			if (!visibleRegion_.contains(col, row)) {
				mouseFocusLock_ = 0;
				return checkMouseOverAndOut(nullptr);
			}
			// set the mouse focus, determine if mouseOver or Out should be generated
			mouseFocus_ = getTransitiveMouseTarget(col, row);
			checkMouseOverAndOut(mouseFocus_);
			screenToWidgetCoordinates(mouseFocus_, col, row);
			mouseFocus_->mouseDown(col, row, button, modifiers);
		}
		mouseClickStart_ = helpers::SteadyClockMillis();
		mouseClickButton_ = button;
		// if the target is different than previous mouse click widget, update it and clear the double click timer
		if (mouseClickWidget_ != mouseFocus_) {
			mouseClickWidget_ = mouseFocus_;
			mouseDoubleClickPrevious_ = 0;
		}
    }

    void RootWindow::mouseUp(int col, int row, MouseButton button, Key modifiers) {
		mouseCoords_ = Point{col, row};
		ASSERT(mouseFocusLock_ > 0);
		mouseFocusLock_--;
		if (mouseFocus_ != nullptr) {
			screenToWidgetCoordinates(mouseFocus_, col, row);
			mouseFocus_->mouseUp(col, row, button, modifiers);
			// see if we have single or double click
			if (mouseClickButton_ == button) {
				size_t end = helpers::SteadyClockMillis();
				if (end - mouseClickStart_ <= MOUSE_CLICK_MAX_DURATION) {
					if (mouseDoubleClickPrevious_ != 0 && mouseClickStart_ - mouseDoubleClickPrevious_ < MOUSE_DOUBLE_CLICK_MAX_INTERVAL) {
						// double click
						mouseFocus_->mouseDoubleClick(col, row, button, modifiers);
						mouseClickWidget_ = nullptr;
					} else {
						// single click
						mouseFocus_->mouseClick(col, row, button, modifiers);
						mouseDoubleClickPrevious_ = end;
					}
				} 
				// that's it - no need to do anything with mouseClick widget here, it has either expired, or it is valid, or it was cleared by a double click happening already
			} 			
		}
    }

    void RootWindow::mouseWheel(int col, int row, int by, Key modifiers) {
		mouseCoords_ = Point{col, row};
		if (mouseFocusLock_ > 0 && mouseFocus_ != nullptr) {
			screenToWidgetCoordinates(mouseFocus_, col, row);
		    mouseFocus_->mouseWheel(col, row, by, modifiers);
		} else {
			if (!visibleRegion_.contains(col, row)) 
				return checkMouseOverAndOut(nullptr);
			Widget * target = getTransitiveMouseTarget(col, row);
			checkMouseOverAndOut(target);
			screenToWidgetCoordinates(target, col, row);
		    target->mouseWheel(col, row, by, modifiers);
		}
    }

    void RootWindow::mouseMove(int col, int row, Key modifiers) {
		mouseCoords_ = Point{col, row};
		if (mouseFocusLock_ > 0 && mouseFocus_ != nullptr) {
			screenToWidgetCoordinates(mouseFocus_, col, row);
		    mouseFocus_->mouseMove(col, row, modifiers);
		} else {
			if (!visibleRegion_.contains(col, row)) 
				return checkMouseOverAndOut(nullptr);
			Widget * target = getTransitiveMouseTarget(col, row);
			checkMouseOverAndOut(target);
			screenToWidgetCoordinates(target, col, row);
			target->mouseMove(col, row, modifiers);
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
            visibleRegion_ = Canvas::VisibleRegion(this);
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

	void RootWindow::registerSelection(SelectionOwner * sender, std::string const & contents) {
		if (renderer_) {
			// if the selection belongs to other widget in the window, let it clear
			if (selectionOwner_ && selectionOwner_ != sender)
			    selectionOwner_->clearSelection();
			// set the selection owner
			selectionOwner_ = sender;
			// let the renderer know the selection value
			renderer_->setSelection(contents);
		} else {
			LOG() << "Set Selection event when no renderer attached";
		}
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