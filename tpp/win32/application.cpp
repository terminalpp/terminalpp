#ifdef WIN32
#include <iostream>

#include <windows.h>

#include "helpers/win32.h"

#include "application.h"


namespace tpp {
	namespace {
		/** Attaches a console to the application for debugging purposes. 
		 */
		void AttachConsole() {
			if (AllocConsole() == 0)
				THROW(helpers::Win32Error("Cannot allocate console"));
			// this is ok, console cannot be detached, so we are fine with keeping the file handles forewer,
			// nor do we need to FreeConsole at any point
			FILE *fpstdin = stdin, *fpstdout = stdout, *fpstderr = stderr;
			// patch the cin, cout, cerr
			freopen_s(&fpstdin, "CONIN$", "r", stdin);
			freopen_s(&fpstdout, "CONOUT$", "w", stdout);
			freopen_s(&fpstderr, "CONOUT$", "w", stderr);
			std::cin.clear();
			std::cout.clear();
			std::cerr.clear();
		}

	} // anonymous namespace


	Application::Application(HINSTANCE hInstance):
	    hInstance_(hInstance) {
		// TODO this should be conditional on a debug flag
		AttachConsole();

	}

	Application::~Application() {

	}

	void Application::mainLoop() {
		MSG msg;
		while (GetMessage(&msg, nullptr, 0, 0) > 0) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			/*			if (msg.wParam == VK_ESCAPE)
							break;
							*/
		}
	}

} // namespace tpp

#endif