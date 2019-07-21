#include "root_window.h"

namespace ui {

	void RootWindow::keyDown(Key k) {
	}

	void RootWindow::keyUp(Key k) {
	}

	void RootWindow::keyChar(helpers::Char c) {
	}

	void RootWindow::mouseDown(unsigned col, unsigned row, MouseButton button, Key modifiers) {

	}
	void RootWindow::mouseUp(unsigned col, unsigned row, MouseButton button, Key modifiers) {

	}
	void RootWindow::mouseWheel(unsigned col, unsigned row, int by, Key modfiers) {

	}
	void RootWindow::mouseMove(unsigned col, unsigned row, Key modifiers) {

	}

	void RootWindow::paste(std::string const& what) {

	}

	void RootWindow::doOnResize(unsigned cols, unsigned rows) {
		resize(cols, rows);
	}




} // namespace vterm