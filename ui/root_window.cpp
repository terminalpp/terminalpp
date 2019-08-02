#include "helpers/time.h"

#include "root_window.h"

namespace ui {

    void RootWindow::mouseDown(int col, int row, MouseButton button, Key modifiers) {
		Widget* target = mouseFocusWidget(col, row);
		Point pos = screenToWidgetCoordinates(target, col, row);
		ASSERT(pos.x >= 0 && pos.y >= 0);
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
		Point pos = screenToWidgetCoordinates(target, col, row);
		ASSERT(pos.x >= 0 && pos.y >= 0);
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
		Point pos = screenToWidgetCoordinates(target, col, row);
		ASSERT(pos.x >= 0 && pos.y >= 0);
		if (target == this)
			Widget::mouseWheel(pos.x, pos.y, by, modifiers);
		else
			target->mouseWheel(pos.x, pos.y, by, modifiers);
    }

    void RootWindow::mouseMove(int col, int row, Key modifiers) {
		Widget* target = mouseFocusWidget(col, row);
		Point pos = screenToWidgetCoordinates(target, col, row);
		ASSERT(pos.x >= 0 && pos.y >= 0);
		if (target == this)
			Widget::mouseMove(pos.x, pos.y, modifiers);
		else
			target->mouseMove(pos.x, pos.y, modifiers);
    }

    void RootWindow::keyChar(helpers::Char c) {
        Container::keyChar(c);
    }

    void RootWindow::keyDown(Key k) {
        Container::keyDown(k);
    }

    void RootWindow::keyUp(Key k) {
        Container::keyUp(k);
    }

    void RootWindow::invalidateContents() {
        Container::invalidateContents();
        visibleRegion_ = Canvas::VisibleRegion(this);
    }

	Widget* RootWindow::mouseFocusWidget(int col, int row) {
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
				mouseFocus_->mouseEnter();
			}
			mouseCol_ = col;
			mouseRow_ = row;
		}
		return mouseFocus_;
	}



} // namespace ui