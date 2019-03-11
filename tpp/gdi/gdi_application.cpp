#include <iostream>

#include <windows.h>
#include <dwrite.h>


#include "../tpp.h"
#include "gdi_application.h"
#include "gdi_terminal_window.h"

namespace tpp {

	using namespace vterm;

	namespace {
		char const * const TerminalWindowClassName = "TerminalWindowClass";
		char const * const TerminalWindowName = "terminal++";

	} // anonymous namespace

	GDIApplication::GDIApplication(HINSTANCE hInstance) :
		hInstance_(hInstance) {
		registerTerminalWindowClass();
		SystemParametersInfo(SPI_SETFONTSMOOTHING,
			TRUE,
			0,
			SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
		SystemParametersInfo(SPI_SETFONTSMOOTHINGTYPE,
			0,
			(PVOID)FE_FONTSMOOTHINGCLEARTYPE,
			SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
		// attaches the console to the terminal for debugging purposes
		attachConsole();
	}

	GDITerminalWindow * GDIApplication::createNewTerminalWindow() {
		// now create the handle, its WM_CREATE
		HWND hwnd = CreateWindowEx(
			WS_EX_LEFT, // the default
			TerminalWindowClassName, // window class
			TerminalWindowName, // window name (all start as terminal++)
			WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, // x position
			CW_USEDEFAULT, // y position
			Settings.defaultWindowWidth,
			Settings.defaultWindowHeight,
			nullptr, // handle to parent
			nullptr, // handle to menu - TODO I should actually create a menu
			hInstance_, // module handle
			nullptr
		);
		ASSERT(hwnd != 0) << "Cannot create window : " << GetLastError();
		return new GDITerminalWindow(hwnd);
	}

	void GDIApplication::mainLoop() {
		MSG msg;
		while (GetMessage(&msg, nullptr, 0, 0) > 0) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
/*			if (msg.wParam == VK_ESCAPE)
				break;
				*/
		}
	}

	void GDIApplication::attachConsole() {
		if (AllocConsole() == 0)
			THROW(Win32Error("Cannot allocate console"));
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


	void GDIApplication::registerTerminalWindowClass() {
		WNDCLASSEX wClass = { 0 };
		wClass.cbSize = sizeof(WNDCLASSEX); // size of the class info
		wClass.hInstance = hInstance_;
		wClass.style = CS_HREDRAW | CS_VREDRAW;
		wClass.lpfnWndProc = GDITerminalWindow::EventHandler; // event handler function for the window class
		wClass.cbClsExtra = 0; // extra memory to be allocated for the class
		wClass.cbWndExtra = 0; // extra memory to be allocated for each window
		wClass.lpszClassName = TerminalWindowClassName; // class name
		wClass.lpszMenuName = nullptr; // menu name
		wClass.hIcon = LoadIcon(nullptr, IDI_APPLICATION); // big icon (alt-tab)
		wClass.hIconSm = LoadIcon(nullptr, IDI_APPLICATION); // small icon (taskbar)
		wClass.hCursor = LoadCursor(nullptr, IDC_IBEAM); // mouse pointer icon
		wClass.hbrBackground = nullptr; // do not display background - the terminal window does it itself
		// register the class
		ATOM result = RegisterClassEx(&wClass);
		ASSERT(result != 0) << "Unable to register window class";
	}

} // namespace tpp