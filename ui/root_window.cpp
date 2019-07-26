#include "root_window.h"

namespace ui {

	unsigned RootWindow::MOUSE_CLICK_MAX_DURATION = 100;

	unsigned RootWindow::MOUSE_DOUBLE_CLICK_MAX_INTERVAL = 300;


	void RootWindow::keyDown(Key k) {
		MARK_AS_UNUSED(k);
	}

	void RootWindow::keyUp(Key k) {
		MARK_AS_UNUSED(k);
	}

	void RootWindow::keyChar(helpers::Char c) {
		MARK_AS_UNUSED(c);
	}

	void RootWindow::mouseDown(unsigned col, unsigned row, MouseButton button, Key modifiers) {
		Widget* target = mouseFocusWidget(col, row);
		Point pos = screenToWidgetCoordinates(target, col, row);
		ASSERT(pos.col >= 0 && pos.row >= 0);
		if (target == this)
			Widget::mouseDown(pos.col, pos.row, button, modifiers);
		else
			target->mouseDown(pos.col, pos.row, button, modifiers);
		// set the mouse click start and button
		mouseClickStart_ = helpers::SteadyClockMillis();
		mouseClickButton_ = button;
		// if the target is different than previous mouse click widget, update it and clear the double click timer
		if (mouseClickWidget_ != target) {
			mouseClickWidget_ = target;
			mouseDoubleClickPrevious_ = 0;
		}
	}

	void RootWindow::mouseUp(unsigned col, unsigned row, MouseButton button, Key modifiers) {
		Widget* target = mouseFocusWidget(col, row);
		Point pos = screenToWidgetCoordinates(target, col, row);
		ASSERT(pos.col >= 0 && pos.row >= 0);
		if (target == this)
			Widget::mouseUp(pos.col, pos.row, button, modifiers);
		else
			target->mouseUp(pos.col, pos.row, button, modifiers);
		// check if we have click
		if (target == mouseClickWidget_ && mouseClickButton_ == button) {
			size_t end = helpers::SteadyClockMillis();
			// it's at least a click, check if it is double click instead
			if (end - mouseClickStart_ <= MOUSE_CLICK_MAX_DURATION) {
				if (mouseDoubleClickPrevious_ != 0 && mouseClickStart_ - mouseDoubleClickPrevious_ <= MOUSE_DOUBLE_CLICK_MAX_INTERVAL) {
					// double click
					target->mouseDoubleClick(pos.col, pos.row, button, modifiers);
					mouseClickWidget_ = nullptr;
					return;
				} else {
					// single click
					target->mouseClick(pos.col, pos.row, button, modifiers);
					mouseDoubleClickPrevious_ = end;
					return;
				}
			}
		} 
		mouseClickWidget_ = nullptr;
	}

	void RootWindow::mouseWheel(unsigned col, unsigned row, int by, Key modifiers) {
		Widget* target = mouseFocusWidget(col, row);
		Point pos = screenToWidgetCoordinates(target, col, row);
		ASSERT(pos.col >= 0 && pos.row >= 0);
		if (target == this)
			Widget::mouseWheel(pos.col, pos.row, by, modifiers);
		else
			target->mouseWheel(pos.col, pos.row, by, modifiers);
	}

	void RootWindow::mouseMove(unsigned col, unsigned row, Key modifiers) {
		Widget* target = mouseFocusWidget(col, row);
		Point pos = screenToWidgetCoordinates(target, col, row);
		ASSERT(pos.col >= 0 && pos.row >= 0);
		if (target == this)
			Widget::mouseMove(pos.col, pos.row, modifiers);
		else
			target->mouseMove(pos.col, pos.row, modifiers);
	}

	void RootWindow::paste(std::string const& what) {
		MARK_AS_UNUSED(what);
	}

	void RootWindow::doOnResize(unsigned cols, unsigned rows) {
		// resize
		updateSize(cols, rows);
		// update the visible region to the new size
		visibleRegion_ = Canvas::VisibleRegion(this);
		// repaint the container
		repaint();
	}

	Widget* RootWindow::mouseFocusWidget(unsigned col, unsigned row) {
		if (mouseFocus_ == nullptr || mouseCol_ != col || mouseRow_ != row) {
			Widget* newFocus = nullptr;
			{
				auto i = lockScreen();
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




} // namespace vterm