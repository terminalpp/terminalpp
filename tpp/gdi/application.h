#pragma once

#include <windows.h>

#include "vterm/virtual_terminal.h"

namespace tpp {

	typedef vterm::VirtualTerminal<vterm::Encoding::UTF16> Terminal;

	class TerminalWindow;

	/** The application encapsulates the renderer and operating system specific details. 
	 */
	class Application {
	public:

		Application(HINSTANCE hInstance);

		TerminalWindow * createNewTerminalWindow(Terminal * terminal);

		void mainLoop();

	private:

		friend class TerminalWindow;

		void registerTerminalWindowClass();

		HINSTANCE const hInstance_;

		WNDCLASSEX terminalWindowClass_;

	};
} // namespace tpp