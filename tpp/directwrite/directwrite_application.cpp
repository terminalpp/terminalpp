#ifdef WIN32
#include "directwrite_application.h"
#include "directwrite_terminal_window.h"

namespace tpp {
	namespace {

		/** Attaches a console to the GDIApplication for debugging purposes.
		 */
		void AttachConsole() {
			OSCHECK(
				AllocConsole()
			) << "Cannot allocate console";
			// this is ok, console cannot be detached, so we are fine with keeping the file handles forewer,
			// nor do we need to FreeConsole at any point
			FILE* fpstdin = stdin, * fpstdout = stdout, * fpstderr = stderr;
			// patch the cin, cout, cerr
			freopen_s(&fpstdin, "CONIN$", "r", stdin);
			freopen_s(&fpstdout, "CONOUT$", "w", stdout);
			freopen_s(&fpstderr, "CONOUT$", "w", stderr);
			std::cin.clear();
			std::cout.clear();
			std::cerr.clear();
		}

		void FixDefaultTerminalWindowProperties(TerminalWindow::Properties& props) {
			vterm::Font defaultFont;
			DirectWriteTerminalWindow::Font* f = DirectWriteTerminalWindow::Font::GetOrCreate(defaultFont, props.fontHeight);
			props.fontWidth = f->widthPx();
		}
	}

	char const* const DirectWriteApplication::TerminalWindowClassName_ = "TerminalWindowClass";

	DirectWriteApplication::DirectWriteApplication(HINSTANCE hInstance) :
		hInstance_(hInstance) {
		// TODO this should be conditional on a debug flag
		AttachConsole();
		registerTerminalWindowClass();
		D2D1_FACTORY_OPTIONS options;
		ZeroMemory(&options, sizeof(D2D1_FACTORY_OPTIONS));
		OSCHECK(SUCCEEDED(D2D1CreateFactory(
			D2D1_FACTORY_TYPE_MULTI_THREADED,
			_uuidof(ID2D1Factory),
			&options,
			&d2dFactory_
		))) << "Unable to create D2D factory";
		OSCHECK(SUCCEEDED(DWriteCreateFactory(
			DWRITE_FACTORY_TYPE_SHARED,
			__uuidof(IDWriteFactory),
			&dwFactory_
		))) << "Unable to create DW factory";
		FixDefaultTerminalWindowProperties(defaultTerminalWindowProperties_);
	}

	DirectWriteApplication::~DirectWriteApplication() {
		// TODO how to release the DirectX factories?
	}

	TerminalWindow* DirectWriteApplication::createTerminalWindow(Session* session, TerminalWindow::Properties const& properties, std::string const& name) {
		return new DirectWriteTerminalWindow(session, properties, name);
	}

	void DirectWriteApplication::mainLoop() {
		MSG msg;
		mainLoopThreadId_ = GetCurrentThreadId();
		while (GetMessage(&msg, nullptr, 0, 0) > 0) {
			if (msg.message == WM_USER && msg.wParam == MSG_BLINK_TIMER) {
				DirectWriteTerminalWindow::BlinkTimer();
			} else {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}

	void DirectWriteApplication::sendBlinkTimerMessage() {
		PostThreadMessage(mainLoopThreadId_, WM_USER, MSG_BLINK_TIMER, 0);
	}

	void DirectWriteApplication::registerTerminalWindowClass() {
		WNDCLASSEX wClass = { 0 };
		wClass.cbSize = sizeof(WNDCLASSEX); // size of the class info
		wClass.hInstance = hInstance_;
		wClass.style = CS_HREDRAW | CS_VREDRAW;
		wClass.lpfnWndProc = DirectWriteTerminalWindow::EventHandler; // event handler function for the window class
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