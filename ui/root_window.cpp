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
		mouseFocusLock_{0},
		mouseFocus_{nullptr},
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

    Widget * RootWindow::mouseDown(int col, int row, MouseButton button, Key modifiers) {
		Widget * target;
		if (++mouseFocusLock_ > 1) {
			target = mouseFocus_;
			if (target != nullptr) {
				screenToWidgetCoordinates(mouseFocus_, col, row);
				mouseFocus_->mouseDown(col, row, button, modifiers);
			}
		} else {
			if (!visibleRegion_.contains(col, row)) {
				mouseFocusLock_ = 0;
				return nullptr;
			}
			target = Container::mouseDown(col, row, button, modifiers);
			mouseFocus_ = target;
		}
		mouseClickStart_ = helpers::SteadyClockMillis();
		mouseClickButton_ = button;
		// if the target is different than previous mouse click widget, update it and clear the double click timer
		if (mouseClickWidget_ != target) {
			mouseClickWidget_ = target;
			mouseDoubleClickPrevious_ = 0;
		}
		// return the mouse target calculated
		return target;
    }

    Widget * RootWindow::mouseUp(int col, int row, MouseButton button, Key modifiers) {
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
		return mouseFocus_;
    }

    void RootWindow::mouseWheel(int col, int row, int by, Key modifiers) {
		if (mouseFocusLock_ > 0 && mouseFocus_ != nullptr) {
			screenToWidgetCoordinates(mouseFocus_, col, row);
		    mouseFocus_->mouseWheel(col, row, by, modifiers);
		} else {
			if (!visibleRegion_.contains(col, row))
			    return;
		    Container::mouseWheel(col, row, by, modifiers);
		}
    }

    void RootWindow::mouseMove(int col, int row, Key modifiers) {
		if (mouseFocusLock_ > 0 && mouseFocus_ != nullptr) {
			screenToWidgetCoordinates(mouseFocus_, col, row);
		    mouseFocus_->mouseMove(col, row, modifiers);
		} else {
			if (!visibleRegion_.contains(col, row))
				return;
			Container::mouseMove(col, row, modifiers);
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

	void RootWindow::requestClipboardPaste(Clipboard * sender) {
		if (pasteRequestTarget_ == nullptr) {
			pasteRequestTarget_ = sender;
			if (renderer_) 
			    renderer_->requestClipboardPaste();
			else
			    LOG << "Paste request w/o renderer";
		} else {
			LOG << "Paste request clash";
		}
	}

	void RootWindow::requestSelectionPaste(Clipboard * sender) {
		if (pasteRequestTarget_ == nullptr) {
			pasteRequestTarget_ = sender;
			if (renderer_) 
			    renderer_->requestSelectionPaste();
			else
			    LOG << "Paste request w/o renderer";
		} else {
			LOG << "Paste request clash";
		}
	}

	void RootWindow::setClipboard(Clipboard * sender, std::string const & contents) {
		MARK_AS_UNUSED(sender);
		// clipboard is simple as there is no state associated with it, we simply inform the renderer that clipboard state should be changed
		if (renderer_) {
			renderer_->setClipboard(contents);
		} else {
			LOG << "Set clipboard event in an unattached root window";
		}
	}

	void RootWindow::setSelection(Clipboard * sender, std::string const & contents) {
		if (renderer_) {
			// if the selection belongs to other widget in the window, let it clear
			if (selectionOwner_ && selectionOwner_ != sender)
			    selectionOwner_->clearSelection();
			// let the renderer know the selection value
			renderer_->setSelection(contents);
			// set the selection owner
			selectionOwner_ = sender;
		} else {
			LOG << "Set Selection event when no renderer attached";
		}
	}

	void RootWindow::clearSelection(Clipboard * sender) {
		ASSERT(sender == selectionOwner_);
		if (renderer_) {
			renderer_->clearSelection();
		} else {
			LOG << "Clear selection event without renderer";
		}
		selectionOwner_ = nullptr;
	}

	void RootWindow::paste(std::string const & contents) {
		if (pasteRequestTarget_) {
		    pasteRequestTarget_->paste(contents);
			pasteRequestTarget_ = nullptr;
		} else {
			LOG << "Paste event received w/o active request";
		}
	}

	void RootWindow::invalidateSelection() {
		if (selectionOwner_) {
			selectionOwner_->invalidateSelection();
			selectionOwner_ = nullptr;
		} else {
			LOG << "invalidate selection w/o selection owner present";
		}
	}




} // namespace ui