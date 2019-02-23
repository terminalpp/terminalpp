#include "application.h"
#include "terminal_window.h"

namespace tpp {

	using namespace vterm;

	namespace {
		char const * const TerminalWindowClassName = "TerminalWindowClass";
		char const * const TerminalWindowName = "terminal++";

	} // anonymous namespace

	Application::Application(HINSTANCE hInstance) :
		hInstance_(hInstance) {
		registerTerminalWindowClass();
	}

	TerminalWindow * Application::createNewTerminalWindow(VirtualTerminal * terminal) {
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
		return new TerminalWindow(hwnd, terminal);
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

	void Application::registerTerminalWindowClass() {
		WNDCLASSEX wClass = { 0 };
		wClass.cbSize = sizeof(WNDCLASSEX); // size of the class info
		wClass.hInstance = hInstance_;
		wClass.style = CS_HREDRAW | CS_VREDRAW;
		wClass.lpfnWndProc = TerminalWindow::EventHandler; // event handler function for the window class
		wClass.cbClsExtra = 0; // extra memory to be allocated for the class
		wClass.cbWndExtra = 0; // extra memory to be allocated for each window
		wClass.lpszClassName = TerminalWindowClassName; // class name
		wClass.lpszMenuName = nullptr; // menu name
		wClass.hIcon = LoadIcon(nullptr, IDI_APPLICATION); // big icon (alt-tab)
		wClass.hIconSm = LoadIcon(nullptr, IDI_APPLICATION); // small icon (taskbar)
		wClass.hCursor = LoadCursor(nullptr, IDC_IBEAM); // mouse pointer icon
		wClass.hbrBackground = (HBRUSH)(COLOR_WINDOWFRAME); // default window color
		// register the class
		ATOM result = RegisterClassEx(&wClass);
		ASSERT(result != 0) << "Unable to register window class";
	}

} // namespace tpp