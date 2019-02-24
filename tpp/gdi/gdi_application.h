#pragma once

#include <windows.h>

#include "vterm/virtual_terminal.h"

#include "../application.h"

namespace tpp {

	/** The application encapsulates the renderer and operating system specific details. 
	 */
	class GDIApplication : public Application {
	public:

		GDIApplication(HINSTANCE hInstance);

		TerminalWindow * createNewTerminalWindow() override;

		void mainLoop();

	private:

		friend class GDITerminalWindow;

		void registerTerminalWindowClass();

		HINSTANCE const hInstance_;

		WNDCLASSEX terminalWindowClass_;

	};
} // namespace tpp