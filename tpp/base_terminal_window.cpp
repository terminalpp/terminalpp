#include "helpers/log.h"

#include "base_terminal_window.h"

namespace tpp {

	void BaseTerminalWindow::keyChar(vterm::Char::UTF8 c) {
		terminal()->keyChar(c);
	}

	void BaseTerminalWindow::keyDown(vterm::Key key) {
		if (key == vterm::Key::Enter + vterm::Key::Alt) {
			setFullscreen(!fullscreen());
		} else if (key == vterm::Key::F5) {
            LOG << "redraw...";
			redraw();
		} else if (key == vterm::Key::F4) {
			if (zoom() == 1)
				setZoom(2);
			else
				setZoom(1);
		} else if (key != vterm::Key::Invalid) {
			terminal()->keyDown(key);
		}
	}

	void BaseTerminalWindow::keyUp(vterm::Key key) {
		terminal()->keyUp(key);
	}

	void BaseTerminalWindow::mouseMove(unsigned x, unsigned y) {
		convertMouseCoordsToCells(x, y);
		if (x != mouseCol_ || y != mouseRow_) {
			mouseCol_ = x;
			mouseRow_ = y;
			terminal()->mouseMove(x, y);
		}
	}

	void BaseTerminalWindow::mouseDown(unsigned x, unsigned y, vterm::MouseButton button) {
		convertMouseCoordsToCells(x, y);
		mouseCol_ = x;
		mouseRow_ = y;
		terminal()->mouseDown(x, y, button);
	}

	void BaseTerminalWindow::mouseUp(unsigned x, unsigned y, vterm::MouseButton button) {
		convertMouseCoordsToCells(x, y);
		mouseCol_ = x;
		mouseRow_ = y;
		terminal()->mouseUp(x, y, button);
	}

	void BaseTerminalWindow::mouseWheel(unsigned x, unsigned y, int offset) {
		convertMouseCoordsToCells(x, y);
		terminal()->mouseWheel(x, y, offset);
	}


	void BaseTerminalWindow::doUpdateBuffer(bool forceDirty) {
		vterm::Terminal::Buffer & b = terminal()->buffer();
		// initialize the first font and colors
		vterm::Color fg;
		vterm::Color bg;
		vterm::Font font;
		{
			vterm::Terminal::Cell& c = b.at(0, 0);
			fg = c.fg;
			bg = c.bg;
			font = DropBlink(c.font);
		}
		doSetForeground(fg);
		doSetBackground(bg);
		doSetFont(font);
		vterm::Terminal::Cursor const & cursor = terminal()->cursor();
		// if cursor state changed, mark the cell containing it as dirty
		bool cursorInRange = cursor.col < cols() && cursor.row < rows();
		if (!forceDirty && cursorInRange)
			b.at(cursor.col, cursor.row).dirty = true;
		// now loop over the entire terminal and update the cells
		for (unsigned r = 0, re = rows(); r < re; ++r) {
			for (unsigned c = 0, ce = cols(); c < ce; ++c) {
				vterm::Terminal::Cell& cell = b.at(c, r);
				if (forceDirty || cell.dirty) {
					cell.dirty = false;
					if (fg != cell.fg) {
						fg = cell.fg;
						doSetForeground(fg);
					}
					if (bg != cell.bg) {
						bg = cell.bg;
						doSetBackground(bg);
					}
					if (font != DropBlink(cell.font)) {
						font = DropBlink(cell.font);
						doSetFont(font);
					}
					doDrawCell(c, r, cell);
				}
			}
		}
		// determine whether cursor should be display and display it if so
		if (cursorInRange && cursor.visible && (blink_ || !cursor.blink)) {
			vterm::Terminal::Cell c = b.at(cursor.col, cursor.row);
			c.fg = cursor.color;
			c.bg = vterm::Color::Black();
			c.c = cursor.character;
			c.font = DropBlink(c.font);
			doDrawCursor(cursor.col, cursor.row, c);
		}
	}

} // namespace tpp