#include "root_window.h"

namespace ui {

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
		if (target == this)
			Widget::mouseDown(col, row, button, modifiers);
		else
			target->mouseDown(col, row, button, modifiers);

	}

	void RootWindow::mouseUp(unsigned col, unsigned row, MouseButton button, Key modifiers) {
		Widget* target = mouseFocusWidget(col, row);
		if (target == this)
			Widget::mouseUp(col, row, button, modifiers);
		else
			target->mouseUp(col, row, button, modifiers);
	}

	void RootWindow::mouseWheel(unsigned col, unsigned row, int by, Key modifiers) {
		Widget* target = mouseFocusWidget(col, row);
		if (target == this)
			Widget::mouseWheel(col, row, by, modifiers);
		else
			target->mouseWheel(col, row, by, modifiers);
	}

	void RootWindow::mouseMove(unsigned col, unsigned row, Key modifiers) {
		Widget* target = mouseFocusWidget(col, row);
		if (target == this)
			Widget::mouseMove(col, row, modifiers);
		else
			target->mouseMove(col, row, modifiers);
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