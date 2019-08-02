#pragma once

#include "../widget.h"
#include "vterm/terminal.h"

namespace ui {

	/** Widget which contains a terminal in it. 

	    A widget which consists of a terminal, whose contents is rendered during the repaint event. This alows simple compositing of various UI elements and the 
	 */
	class Terminal : public Widget {
	public:

	protected:

		/* Mouse events are passed to the terminal. 
		 */
#ifdef HAHA
		void mouseDown(int col, int row, MouseButton button, Key modifiers) override {
			vterm::Terminal::mouseDown(col, row, button, modifiers);
			Widget::mouseDown(col, row, button, modifiers);
		}

		void mouseUp(int col, int row, MouseButton button, Key modifiers) override {
			vterm::Terminal::mouseUp(col, row, button, modifiers);
			Widget::mouseUp(col, row, button, modifiers);
		}

		void mouseWheel(int col, int row, int by, Key modifiers) override {
			vterm::Terminal::mouseWheel(col, row, by, modifiers);
			Widget::mouseWheel(col, row, by, modifiers);
		}

		void mouseMove(int col, int row, Key modifiers) override {
			vterm::Terminal::mouseMove(col, row, modifiers);
			Widget::mouseMove(col, row, modifiers);
		}
#endif

	}; // ui::Terminal


} // namespace ui