#if (defined ARCH_UNIX)

#include "helpers/log.h"
#include "helpers/filesystem.h"

#include "x11_window.h"

#include "x11_application.h"


namespace tpp {

    namespace {

        int X11ErrorHandler(Display * display, XErrorEvent * e) {
            MARK_AS_UNUSED(display);
            LOG() << "X error: " << static_cast<unsigned>(e->error_code);
            return 0;
        }

    } // anonymous namespace


	X11Application::X11Application():
		xDisplay_{nullptr},
		xScreen_{0},
	    xIm_{nullptr},
		selectionOwner_{nullptr} {
        XInitThreads();
		xDisplay_ = XOpenDisplay(nullptr);
		if (xDisplay_ == nullptr) {
			THROW(helpers::Exception()) << "Unable to open X display";
        }
		xScreen_ = DefaultScreen(xDisplay_);

		XSetErrorHandler(X11ErrorHandler);

        // create X Input Method, each window then has its own context
		openInputMethod();

		primaryName_ = XInternAtom(xDisplay_, "PRIMARY", false);
		clipboardName_ = XInternAtom(xDisplay_, "CLIPBOARD", false);
		formatString_ = XInternAtom(xDisplay_, "STRING", false);
		formatStringUTF8_ = XInternAtom(xDisplay_, "UTF8_STRING", false);
		formatTargets_ = XInternAtom(xDisplay_, "TARGETS", false);
		clipboardIncr_ = XInternAtom(xDisplay_, "INCR", false);
		wmDeleteMessage_ = XInternAtom(xDisplay_, "WM_DELETE_WINDOW", false);
		fpsTimerMessage_ = XInternAtom(xDisplay_, "TPP_BLINK_TIMER", false);
		motifWmHints_ = XInternAtom(xDisplay_, "_MOTIF_WM_HINTS", false);
		netWmIcon_ = XInternAtom(xDisplay_, "_NET_WM_ICON", false);

		unsigned long black = BlackPixel(xDisplay_, xScreen_);	/* get color black */
		unsigned long white = WhitePixel(xDisplay_, xScreen_);  /* get color white */
		x11::Window parent = XRootWindow(xDisplay_, xScreen_);
		broadcastWindow_ = XCreateSimpleWindow(xDisplay_, parent, 0, 0, 1, 1, 1, white, black);

		if (
			primaryName_ == x11::None ||
			clipboardName_ == x11::None ||
			formatString_ == x11::None ||
			formatStringUTF8_ == x11::None ||
			formatTargets_ == x11::None ||
			clipboardIncr_ == x11::None ||
			wmDeleteMessage_ == x11::None ||
			fpsTimerMessage_ == x11::None ||
			broadcastWindow_ == x11::None ||
			motifWmHints_ == x11::None ||
			netWmIcon_ == x11::None
		) THROW(helpers::Exception()) << "X11 Atoms instantiation failed";

        fcConfig_ = FcInitLoadConfigAndFonts();

		// start the blinker thread
		X11Window::StartBlinkerThread();
	}

	X11Application::~X11Application() {
		XCloseDisplay(xDisplay_);
		xDisplay_ = nullptr;
	}

	std::string X11Application::getDefaultValidFont() {
		char const * fonts[] = { "Monospace", "DejaVu Sans Mono", "Nimbus Mono", "Liberation Mono", nullptr };
		char const ** f = fonts;
		while (*f != nullptr) {
			std::string found{helpers::Exec(helpers::Command("fc-list", { *f }), "")};
			if (! found.empty())
			    return *f;
			++f;
		}
		return std::string{};
	}

    void X11Application::updateDefaultSettings(helpers::JSON & json) {
		// if no font has been specified, determine default font 
		if (json["font"]["family"].empty() || json["font"]["doubleWidthFamily"].empty()) {
			std::string defaultFont = getDefaultValidFont();
			if (!defaultFont.empty()) {
				if (json["font"]["family"].empty())
					json["font"]["family"] = defaultFont;
				if (json["font"]["doubleWidthFamily"].empty())
					json["font"]["doubleWidthFamily"] = defaultFont;
			} else {
				alert("Cannot guess valid font - please specify manually for best results");
			}
		}
		// add the default shell of the current user
		helpers::JSON & cmd = json["session"]["command"];
		if (cmd.numElements() == 0)
		    cmd.add(helpers::JSON(getpwuid(getuid())->pw_shell));
	}

	std::string X11Application::getSettingsFolder() {
		std::string localSettings(helpers::LocalSettingsDir() + "/terminalpp");
		return localSettings + "/";
	}

    Window * X11Application::createWindow(std::string const & title, int cols, int rows, unsigned cellHeightPx) {
		return new X11Window(title, cols, rows, cellHeightPx);
    }

    void X11Application::xSendEvent(X11Window * window, XEvent & e, long mask) {
		if (window != nullptr)
            XSendEvent(xDisplay_, window->window_, false, mask, &e);
		else 
			XSendEvent(xDisplay_, broadcastWindow_, false, mask, &e);
		XFlush(xDisplay_);
    }

	void X11Application::mainLoop() {
        try {
            XEvent e;
            while (true) { 
                XNextEvent(xDisplay_, &e);
				if (XFilterEvent(&e, x11::None))
					continue;
				X11Window::EventHandler(e);
            }
		} catch (Terminate const& e) {
			// do nothing
		}
	}

	void X11Application::alert(std::string const & message) {
		if (system(STR("xmessage -center \"" << message << "\"").c_str()) != EXIT_SUCCESS) 
            std::cout << message << std::endl;
	}

	void X11Application::openLocalFile(std::string const & filename, bool edit) {
		if (edit) {
			if (system(STR("x-terminal-emulator -e editor \"" << filename << "\" &").c_str()) != EXIT_SUCCESS)
				alert(STR("unable to open file " << filename << " with default editor"));
		} else {
			if (system(STR("xdg-open \"" << filename << "\" &").c_str()) != EXIT_SUCCESS)
				alert(STR("xdg-open not found or unable to open file:\n" << filename));
		}
	}

	void X11Application::openInputMethod() {
		// set the default machine locale instead of the "C" locale
		setlocale(LC_CTYPE, "");
		XSetLocaleModifiers("");
		// create X Input Method, each window then has its own context
		xIm_ = XOpenIM(xDisplay_, nullptr, nullptr, nullptr);
		if (xIm_ != nullptr)
			return;
		XSetLocaleModifiers("@im=local");
		xIm_ = XOpenIM(xDisplay_, nullptr, nullptr, nullptr);
		if (xIm_ != nullptr)
			return;
		XSetLocaleModifiers("@im=");
		xIm_ = XOpenIM(xDisplay_, nullptr, nullptr, nullptr);
		//if (xIm_ != nullptr)
		//	return;
		//OSCHECK(false) << "Cannot open input device (XOpenIM failed)";
	}


} // namespace tpp

#endif