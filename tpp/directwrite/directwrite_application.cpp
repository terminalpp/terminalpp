#include "helpers/helpers.h"
#ifdef ARCH_WINDOWS
#include "directwrite_application.h"
#include "directwrite_terminal_window.h"

namespace tpp {
	namespace {

		/** Attaches a console to the GDIApplication for debugging purposes.

		    However, launching the bypass pty inside wsl would start its own console unless we allocate console at all times. The trick is to create the console, but then hide the window immediately if we are in a release mode.
		 */
		void AttachConsole() {
			OSCHECK(
				AllocConsole()
			) << "Cannot allocate console";
#ifdef NDEBUG
			ShowWindow(GetConsoleWindow(), SW_HIDE);
#endif
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

	}

	wchar_t const* const DirectWriteApplication::TerminalWindowClassName_ = L"TerminalWindowClass";

	DirectWriteApplication::DirectWriteApplication(HINSTANCE hInstance) :
		hInstance_(hInstance) {
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
		start();
	}

	DirectWriteApplication::~DirectWriteApplication() {
		// DirectX factories are release via their WRL COM pointers
	}

	TerminalWindow* DirectWriteApplication::createTerminalWindow(Session* session, TerminalWindow::Properties const& properties, std::string const& name) {
		return new DirectWriteTerminalWindow(session, properties, name);
	}

	std::pair<unsigned, unsigned> DirectWriteApplication::terminalCellDimensions(unsigned fontSize) {
		FontSpec<DWriteFont>* f = FontSpec<DWriteFont>::GetOrCreate(vterm::Font(), fontSize);
		return std::make_pair(f->widthPx(), f->heightPx());
	}

	void DirectWriteApplication::mainLoop() {
		MSG msg;
		mainLoopThreadId_ = GetCurrentThreadId();
		while (GetMessage(&msg, nullptr, 0, 0) > 0) {
			if (msg.message == WM_USER && msg.wParam == MSG_FPS_TIMER) {
				DirectWriteTerminalWindow::FPSTimer();
			} else {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}

	void DirectWriteApplication::sendFPSTimerMessage() {
		PostThreadMessage(mainLoopThreadId_, WM_USER, MSG_FPS_TIMER, 0);
	}

	void DirectWriteApplication::registerTerminalWindowClass() {
		WNDCLASSEXW wClass = { 0 };
		wClass.cbSize = sizeof(WNDCLASSEX); // size of the class info
		wClass.hInstance = hInstance_;
		wClass.style = CS_HREDRAW | CS_VREDRAW;
		wClass.lpfnWndProc = DirectWriteTerminalWindow::EventHandler; // event handler function for the window class
		wClass.cbClsExtra = 0; // extra memory to be allocated for the class
		wClass.cbWndExtra = 0; // extra memory to be allocated for each window
		wClass.lpszClassName = TerminalWindowClassName_; // class name
		wClass.lpszMenuName = nullptr; // menu name
		wClass.hIcon = LoadIcon(hInstance_, L"IDI_ICON1"); // big icon (alt-tab)
		wClass.hIconSm = LoadIcon(hInstance_, L"IDI_ICON1"); // small icon (taskbar)
		wClass.hCursor = LoadCursor(nullptr, IDC_IBEAM); // mouse pointer icon
		wClass.hbrBackground = nullptr; // do not display background - the terminal window does it itself
		// register the class
		ATOM result = RegisterClassExW(&wClass);
		ASSERT(result != 0) << "Unable to register window class";
	}



} // namespace tpp

#endif