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
			visibleRegion_ = Canvas::VisibleRegion(this);
		}

		// terminal interface

		void keyDown(Key k) override;
		void keyUp(Key k) override;
		void keyChar(helpers::Char c) override;

		void mouseDown(unsigned col, unsigned row, MouseButton button, Key modifiers) override;
		void mouseUp(unsigned col, unsigned row, MouseButton button, Key modifiers) override;
		void mouseWheel(unsigned col, unsigned row, int by, Key modfiers) override;
		void mouseMove(unsigned col, unsigned row, Key modifiers) override;

		void paste(std::string const& what) override;

		// widget interface

		using Container::addChild;
		using Container::removeChild;
		using Container::setLayout;

	protected:

		// terminal interface

		void doOnResize(unsigned cols, unsigned rows) override;

		// widget interface

	};


} // namespace uissss