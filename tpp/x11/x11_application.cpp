#ifdef __linux__

#include "helpers/helpers.h"
#include "helpers/log.h"

#include "x11_application.h"
#include "x11_terminal_window.h"

namespace tpp {

    namespace {

        int X11ErrorHandler(Display * display, XErrorEvent * e) {
            MARK_AS_UNUSED(display);
            LOG << "X error: " << static_cast<unsigned>(e->error_code);
            return 0;
        }

    } // anonymous namespace


	X11Application::X11Application():
		xDisplay_(nullptr),
		xScreen_(0) {
        XInitThreads();
		xDisplay_ = XOpenDisplay(nullptr);
		if (xDisplay_ == nullptr)
			THROW(helpers::Exception()) << "Unable to open X display";
		xScreen_ = DefaultScreen(xDisplay_);

		XSetErrorHandler(X11ErrorHandler);

        // set the default machine locale instead of the "C" locale
        setlocale(LC_CTYPE, "");
        XSetLocaleModifiers("");

        // create X Input Method, each window then has its own context
        xIm_ = XOpenIM(xDisplay_, nullptr, nullptr, nullptr);
        ASSERT(xIm_ != nullptr);

		clipboardName_ = XInternAtom(xDisplay_, "CLIPBOARD", false);
		formatString_ = XInternAtom(xDisplay_, "STRING", false);
		formatStringUTF8_ = XInternAtom(xDisplay_, "UTF8_STRING", false);
		formatTargets_ = XInternAtom(xDisplay_, "TARGETS", false);
		clipboardIncr_ = XInternAtom(xDisplay_, "INCR", false);
		wmDeleteMessage_ = XInternAtom(xDisplay_, "WM_DELETE_WINDOW", false);
		blinkTimerMessage_ = XInternAtom(xDisplay_, "TPP_BLINK_TIMER", false);

		unsigned long black = BlackPixel(xDisplay_, xScreen_);	/* get color black */
		unsigned long white = WhitePixel(xDisplay_, xScreen_);  /* get color white */
		Window parent = RootWindow(xDisplay_, xScreen_);
		broadcastWindow_ = XCreateSimpleWindow(xDisplay_, parent, 0, 0, 1, 1, 1, white, black);

		ASSERT(clipboardName_ != None);
		ASSERT(formatString_ != None);
		ASSERT(formatStringUTF8_ != None);
		ASSERT(formatTargets_ != None);
		ASSERT(clipboardIncr_ != None);
		ASSERT(wmDeleteMessage_ != None);
		ASSERT(blinkTimerMessage_ != None);
		ASSERT(broadcastWindow_ != None);
	}

	X11Application::~X11Application() {
		XCloseDisplay(xDisplay_);
		xDisplay_ = nullptr;
	}

	TerminalWindow* X11Application::createTerminalWindow(Session * session, TerminalWindow::Properties const& properties, std::string const& name) {
		return new X11TerminalWindow(session, properties, name);
	}

	std::pair<unsigned, unsigned> X11Application::terminalCellDimensions(unsigned fontSize) {
		FontSpec<XftFont*>* f = FontSpec<XftFont*>::GetOrCreate(vterm::Font(), fontSize);
		return std::make_pair(f->widthPx(), f->heightPx());
	}

    void X11Application::xSendEvent(X11TerminalWindow * window, XEvent & e, long mask) {
		if (window != nullptr)
            XSendEvent(xDisplay_, window->window_, false, mask, &e);
		else 
			XSendEvent(xDisplay_, broadcastWindow_, false, mask, &e);
		XFlush(xDisplay_);
    }

	void X11Application::mainLoop() {
        try {
            XEvent e;
            while (true) { // TODO while terminate
                XNextEvent(xDisplay_, &e);
				if (e.type == ClientMessage && static_cast<unsigned long>(e.xclient.data.l[0]) == blinkTimerMessage_) {
					X11TerminalWindow::BlinkTimer();
				} else {
					if (XFilterEvent(&e, None))
						continue;
					X11TerminalWindow::EventHandler(e);
				}
            }
		} catch (Terminate const& e) {
			// do nothing
			LOG << "Main loop terminated.";
		}
	}

} // namespace tpp

#endif
