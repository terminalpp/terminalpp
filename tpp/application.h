#pragma once

#include "vterm/vterm.h"

namespace tpp {

	class TerminalWindow;

	/** Application provides the common functionality for the terminal application across platforms and renderers, such as settings, persistence, etc. 
	 */
	class Application {
	public:

		virtual ~Application() {
		}

		virtual TerminalWindow * createNewTerminalWindow() = 0;

		virtual void mainLoop() = 0;

	protected:
		Application() {
		}
	};


} // namespace tpp