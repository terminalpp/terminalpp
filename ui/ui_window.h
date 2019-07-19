#pragma once

#include "vterm/terminal.h"

#include "control.h"

namespace vterm {

	/** The UI window, which is a virtual terminal on which the UI elements are drawn. 

	 */
	class UIWindow : public Terminal {

	public:

		void keyDown(Key k) override;
		void keyUp(Key k) override;
		void keyChar(helpers::Char c) override;

		void mouseDown(unsigned col, unsigned row, MouseButton button, Key modifiers) override;
		void mouseUp(unsigned col, unsigned row, MouseButton button, Key modifiers) override;
		void mouseWheel(unsigned col, unsigned row, int by, Key modfiers) override;
		void mouseMove(unsigned col, unsigned row, Key modifiers) override;

		void paste(std::string const& what) override;

	protected:

		void doOnResize(unsigned cols, unsigned rows) override;


	};


} // namespace vterm