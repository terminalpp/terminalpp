#pragma once

#include "vterm/terminal.h"

#include "container.h"

namespace ui {

	typedef vterm::Key Key;

	typedef vterm::MouseButton MouseButton;

	/** The UI window, which is a virtual terminal on which the UI elements are drawn. 

	
	 */
	class RootWindow : public vterm::Terminal, public Container {
	public:

		RootWindow(unsigned width, unsigned height) :
			Terminal(width, height),
			Container(0, 0, width, height) {
		}





		void keyDown(Key k) override;
		void keyUp(Key k) override;
		void keyChar(helpers::Char c) override;

		void mouseDown(unsigned col, unsigned row, MouseButton button, Key modifiers) override;
		void mouseUp(unsigned col, unsigned row, MouseButton button, Key modifiers) override;
		void mouseWheel(unsigned col, unsigned row, int by, Key modfiers) override;
		void mouseMove(unsigned col, unsigned row, Key modifiers) override;

		void paste(std::string const& what) override;

	protected:

		// container's interface

		/** If the valid region of the root window is invalidated, we must update it with the entrety of the terminal. Once the visible region is set, calls the repaint method again, which will now create the appropriate canvas and trigger the doRepaint() method. 
		 */
		void doGetVisibleRegion() {
			visibleRegion_ = Canvas::VisibleRegion(this);
			repaint();
		}

		void doOnResize(unsigned cols, unsigned rows) override;


	};


} // namespace uissss