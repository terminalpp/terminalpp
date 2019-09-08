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

    void RootWindow::mouseDown(int col, int row, MouseButton button, Key modifiers) {
		Widget* target = mouseFocusWidget(col, row);
		// nothing to dispatch if there is no mouse target (i.e. pointer outside of the window and no locked target)
		if (target == nullptr)
		    return;
		++mouseFocusLock_;
		Point pos = screenToWidgetCoordinates(target, col, row);
		if (target == this)
			Widget::mouseDown(pos.x, pos.y, button, modifiers);
		else
			target->mouseDown(pos.x, pos.y, button, modifiers);
		// set the mouse click start and button
		mouseClickStart_ = helpers::SteadyClockMillis();
		mouseClickButton_ = button;
		// if the target is different than previous mouse click widget, update it and clear the double click timer
		if (mouseClickWidget_ != target) {
			mouseClickWidget_ = target;
			mouseDoubleClickPrevious_ = 0;
		}
    }

    void RootWindow::mouseUp(int col, int row, MouseButton button, Key modifiers) {
		Widget* target = mouseFocusWidget(col, row);
		// nothing to dispatch if there is no mouse target (i.e. pointer outside of the window and no locked target)
		if (target == nullptr)
		    return;
		ASSERT(mouseFocusLock_-- > 0);
		Point pos = screenToWidgetCoordinates(target, col, row);
		if (target == this)
			Widget::mouseUp(pos.x, pos.y, button, modifiers);
		else
			target->mouseUp(pos.x, pos.y, button, modifiers);
		// check if we have click
		if (target == mouseClickWidget_ && mouseClickButton_ == button) {
			size_t end = helpers::SteadyClockMillis();
			// it's at least a click, check if it is double click instead
			if (end - mouseClickStart_ <= MOUSE_CLICK_MAX_DURATION) {
				if (mouseDoubleClickPrevious_ != 0 && mouseClickStart_ - mouseDoubleClickPrevious_ <= MOUSE_DOUBLE_CLICK_MAX_INTERVAL) {
					// double click
					target->mouseDoubleClick(pos.x, pos.y, button, modifiers);
					mouseClickWidget_ = nullptr;
					return;
				} else {
					// single click
					target->mouseClick(pos.x, pos.y, button, modifiers);
					mouseDoubleClickPrevious_ = end;
					return;
				}
			}
		} 
		mouseClickWidget_ = nullptr;
    }

    void RootWindow::mouseWheel(int col, int row, int by, Key modifiers) {
		Widget* target = mouseFocusWidget(col, row);
		// nothing to dispatch if there is no mouse target (i.e. pointer outside of the window and no locked target)
		if (target == nullptr)
		    return;
		Point pos = screenToWidgetCoordinates(target, col, row);
		if (target == this)
			Widget::mouseWheel(pos.x, pos.y, by, modifiers);
		else
			target->mouseWheel(pos.x, pos.y, by, modifiers);
    }

    void RootWindow::mouseMove(int col, int row, Key modifiers) {
		Widget* target = mouseFocusWidget(col, row);
		// nothing to dispatch if there is no mouse target (i.e. pointer outside of the window and no locked target)
		if (target == nullptr)
		    return;
		Point pos = screenToWidgetCoordinates(target, col, row);
		if (target == this)
			Widget::mouseMove(pos.x, pos.y, modifiers);
		else
			target->mouseMove(pos.x, pos.y, modifiers);
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

	Widget* RootWindow::mouseFocusWidget(int col, int row) {
		if (mouseFocus_ != nullptr && mouseFocusLock_ > 0)
		    return mouseFocus_;
		if (mouseFocus_ == nullptr || mouseCol_ != col || mouseRow_ != row) {
			Widget* newFocus = nullptr;
			{
				Canvas::Buffer::Ptr ptr(buffer_); // lock the buffer in non priority mode
				newFocus = getMouseTarget(col, row);
			}
			if (mouseFocus_ != newFocus) {
				if (mouseFocus_ != nullptr)
					mouseFocus_->mouseLeave();
				mouseFocus_ = newFocus;
				if (mouseFocus_ != nullptr)
				    mouseFocus_->mouseEnter();
			}
			mouseCol_ = col;
			mouseRow_ = row;
		}
		return mouseFocus_;
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