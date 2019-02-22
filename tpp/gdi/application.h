#pragma once

#include <windows.h>

#include "vterm/screen_buffer.h"

namespace tpp {

	class TerminalWindow;

	/** The application encapsulates the renderer and operating system specific details. 
	 */
	class Application {
	public:

		Application(HINSTANCE hInstance);

		TerminalWindow * createNewTerminalWindow(vterm::ScreenBuffer * screenBuffer);

		void mainLoop();

	private:

		friend class TerminalWindow;

		void registerTerminalWindowClass();

		HINSTANCE const hInstance_;

		WNDCLASSEX terminalWindowClass_;

	};
} // namespace tpp