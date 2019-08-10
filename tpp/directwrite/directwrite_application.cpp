#if (defined ARCH_WINDOWS)

#include <iostream>

#include "helpers/helpers.h"

#include "directwrite_application.h"
#include "directwrite_window.h"

namespace tpp {

	wchar_t const* const DirectWriteApplication::WindowClassName_ = L"TppWindowClass";


    DirectWriteApplication::DirectWriteApplication(HINSTANCE hInstance):
        hInstance_(hInstance) {
        attachConsole();
        registerWindowClass();
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
    }

	Window * DirectWriteApplication::createWindow(std::string const & title, int cols, int rows, unsigned cellHeightPx) {
		return new DirectWriteWindow(title, cols, rows, cellHeightPx);
	}

	std::string DirectWriteApplication::getClipboardContents() {
		std::string result;
		if (OpenClipboard(nullptr)) {
			HANDLE clipboard = GetClipboardData(CF_UNICODETEXT);
			if (clipboard) {
				// ok, on windows wchar_t and char16_t are the same (see helpers/char.h)
				helpers::utf16_char* data = reinterpret_cast<helpers::utf16_char*>(GlobalLock(clipboard));
				if (data) {
					result = helpers::UTF16toUTF8(data);
					GlobalUnlock(clipboard);
				}
			}
			CloseClipboard();
		}
		return result;
	}

    void DirectWriteApplication::mainLoop() {
		MSG msg;
		while (GetMessage(&msg, nullptr, 0, 0) > 0) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
		}
    }

    void DirectWriteApplication::attachConsole() {
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

    void DirectWriteApplication::registerWindowClass() {
		WNDCLASSEXW wClass = { 0 };
		wClass.cbSize = sizeof(WNDCLASSEX); // size of the class info
		wClass.hInstance = hInstance_;
		wClass.style = CS_HREDRAW | CS_VREDRAW;
		wClass.lpfnWndProc = DirectWriteWindow::EventHandler; // event handler function for the window class
		wClass.cbClsExtra = 0; // extra memory to be allocated for the class
		wClass.cbWndExtra = 0; // extra memory to be allocated for each window
		wClass.lpszClassName = WindowClassName_; // class name
		wClass.lpszMenuName = nullptr; // menu name
		wClass.hIcon = LoadIcon(hInstance_, L"IDI_ICON1"); // big icon (alt-tab)
		wClass.hIconSm = LoadIcon(hInstance_, L"IDI_ICON1"); // small icon (taskbar)
		wClass.hCursor = LoadCursor(nullptr, IDC_IBEAM); // mouse pointer icon
		wClass.hbrBackground = nullptr; // do not display background - the terminal window does it itself
		// register the class
		OSCHECK(RegisterClassExW(&wClass) != 0) << "Unable to register window class";
    }


} // namespace tpp

#endif 
