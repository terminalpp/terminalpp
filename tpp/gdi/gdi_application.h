#pragma once

#include <windows.h>

#include "vterm/virtual_terminal.h"

#include "../application.h"

#include "gdi_terminal_window.h"

namespace tpp {


	/** The application encapsulates the renderer and operating system specific details. 
	 */
	class GDIApplication : public Application {
	public:

		GDIApplication(HINSTANCE hInstance);

		GDITerminalWindow * createNewTerminalWindow() override;

		void mainLoop();

	private:

		friend class GDITerminalWindow;

		void registerTerminalWindowClass();

		HINSTANCE const hInstance_;

		WNDCLASSEX terminalWindowClass_;

	};
} // namespace tpp