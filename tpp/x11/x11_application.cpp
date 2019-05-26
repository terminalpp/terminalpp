#ifdef __linux__

#include "helpers/helpers.h"
#include "helpers/log.h"

#include "x11_application.h"
#include "x11_terminal_window.h"

namespace tpp {

    namespace {

        int X11ErrorHandler(Display * display, XErrorEvent * e) {
            LOG << "X error: " << e->error_code;
            return 0;
        }

    } // anonymous namespace

	Display* X11Application::XDisplay_ = nullptr;
	int X11Application::XScreen_ = 0;
    XIM X11Application::XIm_;
	Atom X11Application::ClipboardFormat_;
	Atom X11Application::ClipboardIncr_;

	X11Application::X11Application() {
        XInitThreads();
		ASSERT(XDisplay_ == nullptr) << "X11Application is a singleton";
		XDisplay_ = XOpenDisplay(nullptr);
		if (XDisplay_ == nullptr)
			THROW(helpers::Exception("Unable to open X display"));
		XScreen_ = DefaultScreen(XDisplay_);
        //XSetErrorHandler(X11ErrorHandler);
        //XSynchronize(XDisplay_, true);


        // set the default machine locale instead of the "C" locale
        setlocale(LC_CTYPE, "");
        XSetLocaleModifiers("");

        // create X Input Method, each window then has its own context
        XIm_ = XOpenIM(XDisplay_, nullptr, nullptr, nullptr);
        ASSERT(XIm_ != nullptr);

		ClipboardFormat_ = XInternAtom(XDisplay_, "UTF8_STRING", false);
		ClipboardIncr_ = XInternAtom(XDisplay_, "INCR", false);
		ASSERT(ClipboardFormat_ != None);
		ASSERT(ClipboardIncr_ != None);
	}

	X11Application::~X11Application() {
		XCloseDisplay(XDisplay_);
		XDisplay_ = nullptr;
	}

	void X11Application::mainLoop() {
		XEvent e;
		while (true) {
			XNextEvent(XDisplay_, &e);
            if (XFilterEvent(&e, None))
                continue;
            X11TerminalWindow::EventHandler(e);
       }
	}



} // namespace tpp

#endif