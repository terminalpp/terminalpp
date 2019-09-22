#if (defined ARCH_WINDOWS)

#include <iostream>


#include "helpers/helpers.h"
#include "helpers/filesystem.h"

#include "directwrite_application.h"
#include "directwrite_window.h"

namespace tpp {

	wchar_t const* const DirectWriteApplication::WindowClassName_ = L"TppWindowClass";


    DirectWriteApplication::DirectWriteApplication(HINSTANCE hInstance):
        hInstance_{hInstance},
		selectionOwner_{nullptr} {
        attachConsole();
		// load the icons from the resource file
		iconDefault_ = LoadIcon(hInstance_, L"IDI_ICON1");
		iconNotification_ = LoadIcon(hInstance_, L"IDI_ICON2");
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
		// get the default user locale
		OSCHECK(SUCCEEDED(GetUserDefaultLocaleName(localeName_, LOCALE_NAME_MAX_LENGTH)));
		// get system font fallback
		OSCHECK(SUCCEEDED(static_cast<IDWriteFactory2*>(dwFactory_.Get())->GetSystemFontFallback(&fontFallback_))) << "Unable to create font fallback";
		// and get system font collection
		OSCHECK(SUCCEEDED(dwFactory_->GetSystemFontCollection(&systemFontCollection_, false))) << "Unable to get system font collection";
		// start the blinker thread
		DirectWriteWindow::StartBlinkerThread();
    }

	std::string DirectWriteApplication::isWSLPresent() const {
		helpers::ExitCode ec;
		std::vector<std::string> lines = helpers::SplitAndTrim(helpers::Exec(helpers::Command("wsl.exe", {"-l"}),"", &ec), "\n");
	    if (lines.size() > 1 && lines[0] == "Windows Subsystem for Linux Distributions:") {
			for (size_t i = 1, e = lines.size(); i != e; ++i) {
				if (helpers::EndsWith(lines[i], "(Default)"))
				    return helpers::Split(lines[i], " ")[0];
			}
		}
		// nothing found, return empty string
		return std::string{};
	}

	bool DirectWriteApplication::isBypassPresent() const {
		helpers::ExitCode ec;
		std::string output{helpers::Exec(helpers::Command("wsl.exe", {"--", BYPASS_PATH, "--version"}), "", &ec)};
		return output.find("Terminal++ Bypass, version") == 0;
	}

	bool DirectWriteApplication::installBypass(std::string const & wslDistribution) {
		try {
			std::string url = STR("https://github.com/zduka/tpp-bypass/releases/download/v1.0/tpp-bypass-" << wslDistribution);
			helpers::Exec(helpers::Command("wsl.exe", {"--", "mkdir", "-p", BYPASS_FOLDER}), "");
			helpers::Exec(helpers::Command("wsl.exe", {"--", "wget", "-O", BYPASS_PATH, url}), "");
			helpers::Exec(helpers::Command("wsl.exe", {"--", "chmod", "+x", BYPASS_PATH}), "");
		} catch (...) {
			return false;
		}
		return isBypassPresent();
	}

	void DirectWriteApplication::updateDefaultSettings(helpers::JSON & json) {
		helpers::JSON & cmd = json["session"]["command"];
		if (cmd.numElements() == 0) {
			// if WSL is not present, default to cmd.exe
			std::string wslDefaultDistro{isWSLPresent()};
			if (wslDefaultDistro.empty()) {
				cmd.add(helpers::JSON("cmd.exe"));
				json["session"]["pty"] = "local";
			// otherwise the terminal will default to WSL and we only have to determine whether to use bypass or ConPTY
			} else {
				bool hasBypass = isBypassPresent();
				// if bypass is not present, ask whether it should be installed
				if (!hasBypass) {
					if (MessageBox(nullptr, L"WSL bypass was not found in your default distribution. Do you want terminal++ to install it? (if No, ConPTY will be used instead)", L"WSL Bypass not found", MB_ICONQUESTION + MB_YESNO) == IDYES) {
						hasBypass = installBypass(wslDefaultDistro);
						if (!hasBypass)
						    MessageBox(nullptr, L"Bypass installation failed, most likely due to missing binary for your WSL distribution. Terminal++ will continue with ConPTY.", L"WSL Install bypass failure", MB_ICONSTOP + MB_OK);
						else
						    MessageBox(nullptr, L"WSL Bypass successfully installed", L"Success", MB_ICONINFORMATION + MB_OK);
					}
				}
				if (hasBypass) {
					json["session"]["pty"] = "bypass";
					cmd.add(helpers::JSON("wsl.exe"));
					cmd.add(helpers::JSON("--"));
					cmd.add(helpers::JSON(BYPASS_PATH));
				} else {
					json["session"]["pty"] = "local";
					cmd.add(helpers::JSON("wsl.exe"));
				}
			}
		}
	}

	std::string DirectWriteApplication::getSettingsFolder() {
		std::string localSettings(helpers::LocalSettingsDir() + "\\terminalpp");
		helpers::EnsurePath(localSettings);
		return localSettings + "\\";
	}

	Window * DirectWriteApplication::createWindow(std::string const & title, int cols, int rows, unsigned cellHeightPx) {
		return new DirectWriteWindow(title, cols, rows, cellHeightPx);
	}

    void DirectWriteApplication::mainLoop() {
		MSG msg;
		while (GetMessage(&msg, nullptr, 0, 0) > 0) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
		}
    }

    void DirectWriteApplication::attachConsole() {
		if (AttachConsole(ATTACH_PARENT_PROCESS) == 0) {
			if (GetLastError() != ERROR_INVALID_HANDLE)
			  OSCHECK(false) << "Error when attaching to parent process console";
			// create the console,
		    OSCHECK(AllocConsole()) << "No parent process console and cannot allocate one";
#ifdef NDEBUG
			// and hide the window immediately if we are in release mode
			ShowWindow(GetConsoleWindow(), SW_HIDE);
#endif
		}
        // this is ok, console cannot be detached, so we are fine with keeping the file handles forewer,
        // nor do we need to FreeConsole at any point
        FILE* fpstdin = stdin, * fpstdout = stdout, * fpstderr = stderr;
        // patch the cin, cout, cerr
        OSCHECK(freopen_s(&fpstdin, "CONIN$", "r", stdin) == 0);
        OSCHECK(freopen_s(&fpstdout, "CONOUT$", "w", stdout) == 0);
        OSCHECK(freopen_s(&fpstderr, "CONOUT$", "w", stderr) == 0);
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
		wClass.hIcon = iconDefault_; // big icon (alt-tab)
		wClass.hIconSm = iconDefault_; // small icon (taskbar)
		wClass.hCursor = LoadCursor(nullptr, IDC_IBEAM); // mouse pointer icon
		wClass.hbrBackground = nullptr; // do not display background - the terminal window does it itself
		// register the class
		OSCHECK(RegisterClassExW(&wClass) != 0) << "Unable to register window class";
    }


} // namespace tpp

#endif 
