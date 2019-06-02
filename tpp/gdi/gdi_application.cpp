#ifdef WIN32
#include <iostream>

#include <windows.h>

#include "helpers/win32.h"

#include "gdi_application.h"
#include "gdi_terminal_window.h"


namespace tpp {
	namespace {
		/** Attaches a console to the GDIApplication for debugging purposes. 
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

		void FixDefaultTerminalWindowProperties(TerminalWindow::Properties & props) {
			vterm::Font defaultFont;
			GDITerminalWindow::Font* f = GDITerminalWindow::Font::GetOrCreate(defaultFont, props.fontHeight);
			props.fontWidth = f->widthPx();
	    }

	} // anonymous namespace

	char const * const GDIApplication::TerminalWindowClassName_ = "TerminalWindowClass";

	GDIApplication::GDIApplication(HINSTANCE hInstance):
	    hInstance_(hInstance) {
		FixDefaultTerminalWindowProperties(defaultTerminalWindowProperties_);
		// TODO this should be conditional on a debug flag
		AttachConsole();
		registerTerminalWindowClass();
	}

	GDIApplication::~GDIApplication() {

	}

	TerminalWindow* GDIApplication::createTerminalWindow(TerminalWindow::Properties const& properties, std::string const & name) {
		return new GDITerminalWindow(properties, name);
	}

	void GDIApplication::mainLoop() {
		MSG msg;
		while (GetMessage(&msg, nullptr, 0, 0) > 0) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		// here we are in the on idle mode so we can process queues of all terminals. 
	}

	void GDIApplication::registerTerminalWindowClass() {
		WNDCLASSEX wClass = { 0 };
		wClass.cbSize = sizeof(WNDCLASSEX); // size of the class info
		wClass.hInstance = hInstance_;
		wClass.style = CS_HREDRAW | CS_VREDRAW;
		wClass.lpfnWndProc = GDITerminalWindow::EventHandler; // event handler function for the window class
		wClass.cbClsExtra = 0; // extra memory to be allocated for the class
		wClass.cbWndExtra = 0; // extra memory to be allocated for each window
		wClass.lpszClassName = TerminalWindowClassName_; // class name
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

#endif