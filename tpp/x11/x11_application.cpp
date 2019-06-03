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

		void FixDefaultTerminalWindowProperties(TerminalWindow::Properties & props) {
			vterm::Font defaultFont;
			X11TerminalWindow::Font* f = X11TerminalWindow::Font::GetOrCreate(defaultFont, props.fontHeight);
			props.fontWidth = f->widthPx();
		}

    } // anonymous namespace


	X11Application::X11Application():
		xDisplay_(nullptr),
		xScreen_(0) {
        XInitThreads();
		xDisplay_ = XOpenDisplay(nullptr);
		if (xDisplay_ == nullptr)
			THROW(helpers::Exception("Unable to open X display"));
		xScreen_ = DefaultScreen(xDisplay_);

		// this needs to be done *after* the display & screen are initialized
		FixDefaultTerminalWindowProperties(defaultTerminalWindowProperties_);


		//XSetErrorHandler(X11ErrorHandler);
        //XSynchronize(XDisplay_, true);


        // set the default machine locale instead of the "C" locale
        setlocale(LC_CTYPE, "");
        XSetLocaleModifiers("");

        // create X Input Method, each window then has its own context
        xIm_ = XOpenIM(xDisplay_, nullptr, nullptr, nullptr);
        ASSERT(xIm_ != nullptr);

		clipboardFormat_ = XInternAtom(xDisplay_, "UTF8_STRING", false);
		clipboardIncr_ = XInternAtom(xDisplay_, "INCR", false);
		ASSERT(clipboardFormat_ != None);
		ASSERT(clipboardIncr_ != None);

	}

	X11Application::~X11Application() {
		XCloseDisplay(xDisplay_);
		xDisplay_ = nullptr;
	}

	TerminalWindow* X11Application::createTerminalWindow(TerminalWindow::Properties const& properties, std::string const& name) {
		return new X11TerminalWindow(properties, name);
	}


	void X11Application::mainLoop() {
		XEvent e;
		while (true) {
			XNextEvent(xDisplay_, &e);
            if (XFilterEvent(&e, None))
                continue;
            X11TerminalWindow::EventHandler(e);
       }
	}



} // namespace tpp

#endif