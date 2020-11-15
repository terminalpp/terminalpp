#if (defined ARCH_UNIX && defined RENDERER_NATIVE)

#include "helpers/filesystem.h"
#include "helpers/time.h"

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
        mainLoopRunning_{false},
	    xIm_{nullptr}, 
        selectionOwner_{nullptr} {
        XInitThreads();
		xDisplay_ = XOpenDisplay(nullptr);
		if (xDisplay_ == nullptr) 
			THROW(Exception()) << "Unable to open X display";
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
		xAppEvent_ = XInternAtom(xDisplay_, "_APP_EVT", false);
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
			xAppEvent_ == x11::None ||
			broadcastWindow_ == x11::None ||
			motifWmHints_ == x11::None ||
			netWmIcon_ == x11::None
		) THROW(Exception()) << "X11 Atoms instantiation failed";

        fcConfig_ = FcInitLoadConfigAndFonts();

        // create mouse cursors
        cursorArrow_ = XcursorLibraryLoadCursor(xDisplay_, "top_left_arrow");        
        cursorHand_ = XcursorLibraryLoadCursor(xDisplay_, "hand2");        
        cursorBeam_ = XcursorLibraryLoadCursor(xDisplay_, "xterm");        
        cursorVerticalSize_ = XcursorLibraryLoadCursor(xDisplay_, "sb_v_double_arrow");        
        cursorHorizontalSize_ = XcursorLibraryLoadCursor(xDisplay_, "sb_h_double_arrow");        
        cursorWait_ = XcursorLibraryLoadCursor(xDisplay_, "watch");        
        cursorForbidden_ = XcursorLibraryLoadCursor(xDisplay_, "X_cursor");        

		X11Window::StartBlinkerThread();
    }

	X11Application::~X11Application() {
		XCloseDisplay(xDisplay_);
		xDisplay_ = nullptr;
	}

    void X11Application::alert(std::string const & message) {
		if (system(STR("xmessage -center \"" << message << "\"").c_str()) != EXIT_SUCCESS) 
            std::cout << message << std::endl;
    }

    bool X11Application::query(std::string const & title, std::string const & message) {
        return (system(STR("xmessage -buttons Yes:0,No:1,Cancel:1 -center \"" << title << "\n" << message << "\"").c_str()) == 0);
    }

    void X11Application::openLocalFile(std::string const & filename, bool edit) {
		if (edit) {
			if (system(STR("x-terminal-emulator -e editor \"" << filename << "\" &").c_str()) == EXIT_SUCCESS)
                return;
		} else {
			if (system(STR("xdg-open \"" << filename << "\" &").c_str()) == EXIT_SUCCESS)
                return;
		}
        if (query("Unable to open file with default viewer/editor", STR("Cannot open file " << filename << ". Do you want to copy its path to clipboard so that you can do that manually?")))
            setClipboard(filename);
    }

    void X11Application::openUrl(std::string const & url) {
			if (system(STR("xdg-open \"" << url << "\" &").c_str()) != EXIT_SUCCESS)
				alert(STR("xdg-open not found or unable to open url " << url));
    }

    void X11Application::setClipboard(std::string const & contents) {
        // let the app manage the clipboard requests from other windows now
        clipboard_ = contents;
        // inform X that we own the clipboard selection
		XSetSelectionOwner(xDisplay_, clipboardName_, broadcastWindow_, CurrentTime);
        if (! mainLoopRunning_) {
            XEvent e;
            Stopwatch stw{ /* start */ true };
            do {
                while (!XCheckTypedWindowEvent(xDisplay_, broadcastWindow_, SelectionRequest , &e)) {
                    // check if we have timeout w/o the clipboard request
                    if (stw.value() >= SET_CLIPBOARD_TIMEOUT)
                        return;
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
                processXEvent(e);
                // use the whole timeout
                } while (true);
                //e.type != SelectionRequest || e.xselectionrequest.selection != clipboardName_);
        }
    }

    Window * X11Application::createWindow(std::string const & title, int cols, int rows) {
		return new X11Window{title, cols, rows, eventQueue_};
    }

    void X11Application::mainLoop() {
        XEvent e;
        mainLoopRunning_ = true;
        try {
            while (true) { 
                XNextEvent(xDisplay_, &e);
                processXEvent(e);
            }
        } catch (TerminateException const &) {
            // don't do anything
        }
        mainLoopRunning_ = false;
    }

    void X11Application::xSendEvent(X11Window * window, XEvent & e, long mask) {
		if (window != nullptr) {
            XSendEvent(xDisplay_, window->window_, false, mask, &e);
        } else {
            e.xany.window = broadcastWindow_;
			XSendEvent(xDisplay_, broadcastWindow_, false, mask, &e);
        }
		XFlush(xDisplay_);
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

    void X11Application::processXEvent(XEvent & e) {
        if (XFilterEvent(&e, x11::None))
            return;
        switch (e.type) {
			/** If we lose ownership, clear the clipboard contents with the application, or if we lose primary ownership, just clear the selection.   
			 */
            case SelectionClear: {
                if (e.xselectionclear.selection == clipboardName_) {
    				clipboard_.clear();
                } else if (selectionOwner_ != nullptr) {
                    X11Window * owner = selectionOwner_;
                    selectionOwner_ = nullptr;
                    selection_.clear();
                    // clears the selection in the renderer without triggering any X events
                    owner->clearSelection(nullptr);
                }
				break;
            }
			/** Called when the clipboard contents is requested by an outside app. 
			 */
            case SelectionRequest: {
				XSelectionEvent response;
				response.type = SelectionNotify;
				response.requestor = e.xselectionrequest.requestor;
				response.selection = e.xselectionrequest.selection;
				response.target = e.xselectionrequest.target;
				response.time = e.xselectionrequest.time;
				// by default, the request is rejected
				response.property = x11::None; 
				// if the target is TARGETS, then all supported formats should be sent, in our case this is simple, only UTF8_STRING is supported
				if (response.target == formatTargets_) {
					XChangeProperty(
						xDisplay_,
						e.xselectionrequest.requestor,
						e.xselectionrequest.property,
						e.xselectionrequest.target,
						32, // atoms are 4 bytes, so 32 bits
						PropModeReplace,
						reinterpret_cast<unsigned char const*>(&formatStringUTF8_),
						1
					);
					response.property = e.xselectionrequest.property;
				// otherwise, if UTF8_STRING, or a STRING is requested, we just send what we have 
				} else if (response.target == formatString_ || response.target == formatStringUTF8_) {
                    std::string clipboard = (response.selection == clipboardName_) ? clipboard_ : selection_;
					XChangeProperty(
						xDisplay_,
						e.xselectionrequest.requestor,
						e.xselectionrequest.property,
						e.xselectionrequest.target,
						8, // utf-8 is encoded in chars, i.e. 8 bits
						PropModeReplace,
						reinterpret_cast<unsigned char const *>(clipboard.c_str()),
						clipboard.size()
					);
					response.property = e.xselectionrequest.property;
				}
				// send the event to the requestor
				if (!XSendEvent(
					e.xselectionrequest.display,
					e.xselectionrequest.requestor,
					1, // propagate
					0, // event mask
					reinterpret_cast<XEvent*>(&response)
				))
					LOG() << "Error sending selection notify";
				break;
            }
            case ClientMessage:
                if (e.xany.window == broadcastWindow_) {
                    if (static_cast<unsigned long>(e.xclient.message_type) == xAppEvent_) 
                        eventQueue_.processEvent();
                    break;
                }
                // fallthrough
            default:
                X11Window::EventHandler(e);
        }
    }

}

#endif