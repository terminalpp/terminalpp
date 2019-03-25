#pragma once

#include "vterm/renderer.h"

namespace tpp {

	/** Implementation of a single terminal window. 

	    The terminal window is a vterm renderer that can display the contents of the terminal. 
	 */
	class TerminalWindow : public vterm::Renderer {
	public:

	protected:
		/** Called when the terminal is to be repainted. 
		 */
		virtual void repaint(vterm::Terminal::RepaintEvent & e);

	};


} // namespace tpp