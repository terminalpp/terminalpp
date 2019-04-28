#include "helpers/log.h"

#include "base_terminal_window.h"

namespace tpp {

	void BaseTerminalWindow::sendChar(vterm::Char::UTF8 c) {
		terminal()->sendChar(c);
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

	void BaseTerminalWindow::doUpdateBuffer(bool forceDirty) {
		vterm::Terminal::Layer l = terminal()->getDefaultLayer();
		// initialize the first font and colors
		vterm::Color fg;
		vterm::Color bg;
		vterm::Font font;
		{
			vterm::Cell& c = l->at(0, 0);
			fg = c.fg;
			bg = c.bg;
			font = DropBlink(c.font);
		}
		doSetForeground(fg);
		doSetBackground(bg);
		doSetFont(font);
		// if cursor state changed, mark the cell containing it as dirty
		helpers::Point cp = terminal()->cursorPos();
		bool cursorInRange = cp.col < cols() && cp.row < rows();
		if (!forceDirty && cursorInRange)
			l->at(cp.col, cp.row).dirty = true;
		// now loop over the entire terminal and update the cells
		for (unsigned r = 0, re = rows(); r < re; ++r) {
			for (unsigned c = 0, ce = cols(); c < ce; ++c) {
				vterm::Cell& cell = l->at(c, r);
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
		if (cursorInRange && terminal()->cursorVisible() && (blink_ || !terminal()->cursorBlink())) {
			vterm::Cell c = l->at(cp.col, cp.row);
			// TODO these should be selected somewhere!
			c.fg = vterm::Color::White();
			c.bg = vterm::Color::Black();
			c.c = terminal()->cursorCharacter();
			c.font = DropBlink(c.font);
			doDrawCursor(cp.col, cp.row, c);
		}
	}

} // namespace tpp