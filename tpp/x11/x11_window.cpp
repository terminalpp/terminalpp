#if (defined ARCH_UNIX)

#include "helpers/log.h"

#include "x11_window.h"

namespace tpp {

	/** The statically generated icon description stored in an array so that in can be part of the executable. 
	
	    To change its contents, run the `icons` build target. 
	 */
	extern unsigned long tppIcon[];
	extern unsigned long tppIconNotification[];


    X11Window::X11Window(std::string const & title, int cols, int rows, unsigned baseCellHeightPx):
        RendererWindow(cols, rows, Font::GetOrCreate(ui::Font(), 0, baseCellHeightPx)->widthPx(),baseCellHeightPx) ,
		display_(X11Application::Instance()->xDisplay()),
		screen_(X11Application::Instance()->xScreen()),
	    visual_(DefaultVisual(display_, screen_)),
	    colorMap_(DefaultColormap(display_, screen_)),
		ic_(nullptr),
	    buffer_(0),
	    draw_(nullptr),
        text_(nullptr),
        textSize_(0),
        pasteTarget_{nullptr} {
		unsigned long black = BlackPixel(display_, screen_);	/* get color black */
		unsigned long white = WhitePixel(display_, screen_);  /* get color white */
        x11::Window parent = XRootWindow(display_, screen_);

		window_ = XCreateSimpleWindow(display_, parent, 0, 0, widthPx_, heightPx_, 1, white, black);

		// from http://math.msu.su/~vvb/2course/Borisenko/CppProjects/GWindow/xintro.html

		/* here is where some properties of the window can be set.
			   The third and fourth items indicate the name which appears
			   at the top of the window and the name of the minimized window
			   respectively.
			*/
		XSetStandardProperties(display_, window_, title.c_str(), nullptr, x11::None, nullptr, 0, nullptr);

		/* this routine determines which types of input are allowed in
		   the input.  see the appropriate section for details...
		*/
		XSelectInput(display_, window_, ButtonPressMask | ButtonReleaseMask | PointerMotionMask | KeyPressMask | KeyReleaseMask | StructureNotifyMask | VisibilityChangeMask | ExposureMask | FocusChangeMask);

		/* X11 in itself does not deal with window close requests, but this enables sending of the WM_DELETE_WINDOW message when the close button is send and the application can decide what to do instead. 

		   The message is received as a client message with the wmDeleteMessage_ atom in its first long payload.
		 */
		XSetWMProtocols(display_, window_, & X11Application::Instance()->wmDeleteMessage_, 1);

        XGCValues gcv;
        memset(&gcv, 0, sizeof(XGCValues));
    	gcv.graphics_exposures = False;
        gc_ = XCreateGC(display_, parent, GCGraphicsExposures, &gcv);

		// only create input context if XIM is present
		if (X11Application::Instance()->xIm_ != nullptr) {
			// create input context for the window... The extra arguments to the XCreateIC are c-c c-v from the internet and for now are a mystery to me
			ic_ = XCreateIC(X11Application::Instance()->xIm_, XNInputStyle, XIMPreeditNothing | XIMStatusNothing,
				XNClientWindow, window_, XNFocusWindow, window_, nullptr);
		}

        updateXftStructures(cols_);

        // set the icon
		setIcon(ui::RootWindow::Icon::Default);

		// register the window
        AddWindowNativeHandle(this, window_);
    }

    X11Window::~X11Window() {
        RemoveWindow(window_);
		XFreeGC(display_, gc_);
        delete [] text_;
    }

    void X11Window::setIcon(ui::RootWindow::Icon icon) {
        /* 
		    The window icon must be an array of BGRA colors for the different icon sizes where the first element is the total size of the array followed by arbitrary icon sizes encoded by first 2 items representing the icon width and height followed by the actual pixels. 
         */
        unsigned long * iconHandle;
        switch (icon) {
            case ui::RootWindow::Icon::Notification:
                iconHandle = tppIconNotification;
                break;
            default:
                iconHandle = tppIcon;
                break;
        }
		XChangeProperty(
			display_,
			window_,
			X11Application::Instance()->netWmIcon_,
			XA_CARDINAL,
			32,
			PropModeReplace,
			reinterpret_cast<unsigned char*>(&iconHandle[1]),
			iconHandle[0]
		);
    }

    void X11Window::show() {
        XMapWindow(display_, window_);
    }

    void X11Window::updateFullscreen(bool value) {
        X11Application * app = X11Application::Instance();
		MotifHints hints;
		hints.flags = 2;
		if (value == true) {
            // get window size & position
            XWindowAttributes attrs;
            x11::Window childW; 
			XGetWindowAttributes(display_, window_, & attrs);
            XTranslateCoordinates(display_, window_, DefaultRootWindow(display_), 0, 0, & fullscreenRestore_.x, & fullscreenRestore_.y, &childW);
			hints.decorations = 0;
            fullscreenRestore_.width = attrs.width;
            fullscreenRestore_.height = attrs.height;
            fullscreenRestore_.x -= attrs.x;
            fullscreenRestore_.y -= attrs.y;
            // remove the decorations
            XChangeProperty(display_, window_, app->motifWmHints_, app->motifWmHints_, 32, PropModeReplace, (unsigned char*)& hints, 5);
            // update window size and position
            Screen * screen = XScreenOfDisplay(display_, XDefaultScreen(display_));
    		XMoveResizeWindow(display_, window_, 0, 0, XWidthOfScreen(screen), XHeightOfScreen(screen));
		} else {
			hints.decorations = 1;
            XChangeProperty(display_, window_, app->motifWmHints_, app->motifWmHints_, 32, PropModeReplace, (unsigned char*)& hints, 5);
    		XMoveResizeWindow(
                display_,
                window_,
                fullscreenRestore_.x,
                fullscreenRestore_.y,
                fullscreenRestore_.width,
                fullscreenRestore_.height
            );
		}
		XMapWindow(display_, window_);
        // actually update the value
        Window::updateFullscreen(value);
    }

    void X11Window::updateZoom(double value) {
		Font * f = Font::GetOrCreate(ui::Font(), 0, static_cast<unsigned>(baseCellHeightPx_ * value));
		cellWidthPx_ = f->widthPx();
		cellHeightPx_ = f->heightPx();
		Window::updateZoom(value);
		Window::updateSizePx(widthPx_, heightPx_);
    }

    void X11Window::requestClipboardPaste() {
        X11Application * app = X11Application::Instance();
		XConvertSelection(display_, app->clipboardName_, app->formatStringUTF8_, app->clipboardName_, window_, CurrentTime);
    }

    void X11Window::requestSelectionPaste() {
        X11Application * app = X11Application::Instance();
		XConvertSelection(display_, app->primaryName_, app->formatStringUTF8_, app->primaryName_, window_, CurrentTime);
    }


    void X11Window::setClipboard(std::string const & contents) {
        X11Application * app = X11Application::Instance();
        // let the app manage the clipboard requests from other windows now
        app->clipboard_ = contents;
        // inform X that we own the clipboard selection
		XSetSelectionOwner(display_, app->clipboardName_, window_, CurrentTime);
    }

    void X11Window::setSelection(std::string const & contents) {
        X11Application * app = X11Application::Instance();
        // if there is a selection ownership by other window, let it invalidate its selection first
        if (app->selectionOwner_)
            app->selectionOwner_->invalidateSelection();
        // set the contents
		app->selection_ = contents;
		app->selectionOwner_ = this;
        // inform X that we own the clipboard selection
        XSetSelectionOwner(display_, app->primaryName_, window_, CurrentTime);
    }

    void X11Window::clearSelection() {
        X11Application * app = X11Application::Instance();
        if (app->selectionOwner_ == this) {
            app->selectionOwner_ = nullptr;
            app->selection_.clear();
            // tell the x server that the selection was cleared
            XSetSelectionOwner(display_, app->primaryName_, x11::None, CurrentTime);
        } else {
			LOG << "Window renderer clear selection does not match stored selection owner.";
        }
    }

    void X11Window::yieldSelection() {
        X11Application * app = X11Application::Instance();
        // if the app already does not know about any selection, the event can be ignored
        if (app->selectionOwner_ != nullptr) {
            // tell the owner of the selection to invalidate its selection
            app->selectionOwner_->invalidateSelection();
            // invalidate the selection in the app
            app->selectionOwner_ = nullptr;
            app->selection_.clear();
        }
    }

    unsigned X11Window::GetStateModifiers(int state) {
		unsigned modifiers = 0;
		if (state & 1)
			modifiers += ui::Key::Shift;
		if (state & 4)
			modifiers += ui::Key::Ctrl;
		if (state & 8)
			modifiers += ui::Key::Alt;
		if (state & 64)
			modifiers += ui::Key::Win;
		return modifiers;
    }

    ui::Key X11Window::GetKey(KeySym k, unsigned modifiers, bool pressed) {
        if (k >= 'a' && k <= 'z') 
            return ui::Key(k - 32, modifiers);
        if (k >= 'A' && k <= 'Z')
            return ui::Key(k, modifiers);
        if (k >= '0' && k <= '9')
            return ui::Key(k, modifiers);
        // numpad
        if (k >= XK_0 && k <= XK_9)
            return ui::Key(ui::Key::Numpad0 + k - XK_0, modifiers);
        // fn keys
        if (k >= XK_F1 && k <= XK_F12)
            return ui::Key(ui::Key::F1 + k - XK_F1, modifiers);
        // others
        switch (k) {
            case XK_BackSpace:
                return ui::Key(ui::Key::Backspace, modifiers);
            case XK_Tab:
                return ui::Key(ui::Key::Tab, modifiers);
            case XK_Return:
                return ui::Key(ui::Key::Enter, modifiers);
            case XK_Caps_Lock:
                return ui::Key(ui::Key::CapsLock, modifiers);
            case XK_Escape:
                return ui::Key(ui::Key::Esc, modifiers);
            case XK_space:
                return ui::Key(ui::Key::Space, modifiers);
            case XK_Page_Up:
                return ui::Key(ui::Key::PageUp, modifiers);
            case XK_Page_Down:
                return ui::Key(ui::Key::PageDown, modifiers);
            case XK_End:
                return ui::Key(ui::Key::End, modifiers);
            case XK_Home:
                return ui::Key(ui::Key::Home, modifiers);
            case XK_Left:
                return ui::Key(ui::Key::Left, modifiers);
            case XK_Up:
                return ui::Key(ui::Key::Up, modifiers);
            case XK_Right:
                return ui::Key(ui::Key::Right, modifiers);
            case XK_Down:
                return ui::Key(ui::Key::Down, modifiers);
            case XK_Insert:
                return ui::Key(ui::Key::Insert, modifiers);
            case XK_Delete:
                return ui::Key(ui::Key::Delete, modifiers);
            case XK_Menu:
                return ui::Key(ui::Key::Menu, modifiers);
            case XK_KP_Multiply:
                return ui::Key(ui::Key::NumpadMul, modifiers);
            case XK_KP_Add:
                return ui::Key(ui::Key::NumpadAdd, modifiers);
            case XK_KP_Separator:
                return ui::Key(ui::Key::NumpadComma, modifiers);
            case XK_KP_Subtract:
                return ui::Key(ui::Key::NumpadSub, modifiers);
            case XK_KP_Decimal:
                return ui::Key(ui::Key::NumpadDot, modifiers);
            case XK_KP_Divide:
                return ui::Key(ui::Key::NumpadDiv, modifiers);
            case XK_Num_Lock:
                return ui::Key(ui::Key::NumLock, modifiers);
            case XK_Scroll_Lock:
                return ui::Key(ui::Key::ScrollLock, modifiers);
            case XK_semicolon:
                return ui::Key(ui::Key::Semicolon, modifiers);
            case XK_equal:
                return ui::Key(ui::Key::Equals, modifiers);
            case XK_comma:
                return ui::Key(ui::Key::Comma, modifiers);
            case XK_minus:
                return ui::Key(ui::Key::Minus, modifiers);
            case XK_period: // .
                return ui::Key(ui::Key::Dot, modifiers);
            case XK_slash:
                return ui::Key(ui::Key::Slash, modifiers);
            case XK_grave: // `
                return ui::Key(ui::Key::Tick, modifiers);
            case XK_bracketleft: // [
                return ui::Key(ui::Key::SquareOpen, modifiers);
            case XK_backslash:  
                return ui::Key(ui::Key::Backslash, modifiers);
            case XK_bracketright: // ]
                return ui::Key(ui::Key::SquareClose, modifiers);
            case XK_apostrophe: // '
                return ui::Key(ui::Key::Quote, modifiers);
			case XK_Shift_L:
			case XK_Shift_R:
				if (pressed)
					modifiers |= ui::Key::Shift;
				else
					modifiers &= ~ui::Key::Shift;
				return ui::Key(ui::Key::ShiftKey, modifiers);
			case XK_Control_L:
			case XK_Control_R:
				if (pressed)
					modifiers |= ui::Key::Ctrl;
				else
					modifiers &= ~ui::Key::Ctrl;
				return ui::Key(ui::Key::CtrlKey, modifiers);
			case XK_Alt_L:
			case XK_Alt_R:
				if (pressed)
					modifiers |= ui::Key::Alt;
				else
					modifiers &= ~ui::Key::Alt;
				return ui::Key(ui::Key::AltKey, modifiers);
			case XK_Meta_L:
			case XK_Meta_R:
				if (pressed)
					modifiers |= ui::Key::Win;
				else
					modifiers &= ~ui::Key::Win;
				return ui::Key(ui::Key::WinKey, modifiers);
			default:
                return ui::Key(ui::Key::Invalid, 0);
        }
    }

    void X11Window::EventHandler(XEvent & e) {
        X11Window * window = GetWindowFromNativeHandle(e.xany.window);
        switch(e.type) {
            /* Handles repaint event when window is shown or a repaint was triggered. 
             */
            case Expose: 
                if (e.xexpose.count != 0)
                    break;
                window->paint();
                break;
			/** Handles when the window gets focus. 
			 */
			case FocusIn:
				if (e.xfocus.mode == NotifyGrab || e.xfocus.mode == NotifyUngrab)
					break;
				ASSERT(window != nullptr);
				window->setFocus(true);
				break;
			/** Handles when the window loses focus. 
			 */
			case FocusOut:
				if (e.xfocus.mode == NotifyGrab || e.xfocus.mode == NotifyUngrab)
					break;
				ASSERT(window != nullptr);
				window->setFocus(false);
				break;
            /* Handles window resize, which should change the terminal size accordingly. 
             */  
            case ConfigureNotify: {
                if (window->widthPx_ != static_cast<unsigned>(e.xconfigure.width) || window->heightPx_ != static_cast<unsigned>(e.xconfigure.height))
                    window->updateSizePx(e.xconfigure.width, e.xconfigure.height);
                break;
            }
            case MapNotify:
                break;
            /* Unlike Win32 we have to determine whether we are dealing with sendChar, or keyDown. 
             */
            case KeyPress: {
				unsigned modifiers = GetStateModifiers(e.xkey.state);
				window->activeModifiers_ = ui::Key(ui::Key::Invalid, modifiers);
				KeySym kSym;
                char str[32];
                Status status;
				int strLen = 0;
				if (window->ic_ != nullptr)
					strLen = Xutf8LookupString(window->ic_, &e.xkey, str, sizeof str, &kSym, &status);
				else
					strLen = XLookupString(&e.xkey, str, sizeof str, &kSym, nullptr);
                // if it is printable character and there were no modifiers other than shift pressed, we are dealing with printable character (backspace is not printable character)
                if (strLen > 0 && (str[0] < 0 || str[0] >= 0x20) && (e.xkey.state & 0x4c) == 0 && str[0] != 0x7f) {
                    char * x = reinterpret_cast<char*>(& str);
					helpers::Char const* c = helpers::Char::At(x, x + 32);
					if (c != nullptr) {
						window->keyChar(*c);
					    break;
                    }
                }
                // otherwise if the keysym was recognized, it is a keyDown event
                ui::Key key = GetKey(kSym, modifiers, true);
				// if the modifiers were updated (i.e. the key is Shift, Ctrl, Alt or Win, updated active modifiers
				if (modifiers != key.modifiers())
					window->activeModifiers_ = ui::Key(ui::Key::Invalid, modifiers);
				if (key != ui::Key::Invalid)
                    window->keyDown(key);
                break;
            }
            case KeyRelease: {
				unsigned modifiers = GetStateModifiers(e.xkey.state);
				window->activeModifiers_ = ui::Key(ui::Key::Invalid, modifiers);
				KeySym kSym = XLookupKeysym(& e.xkey, 0);
                ui::Key key = GetKey(kSym, modifiers, false);
				// if the modifiers were updated (i.e. the key is Shift, Ctrl, Alt or Win, updated active modifiers
				if (modifiers != key.modifiers())
					window->activeModifiers_ = ui::Key(ui::Key::Invalid, modifiers);
				if (key != ui::Key::Invalid)
                    window->keyUp(key);
                break;
            }
            case ButtonPress: 
				window->activeModifiers_ = ui::Key(ui::Key::Invalid, GetStateModifiers(e.xbutton.state));
				switch (e.xbutton.button) {
                    case 1:
                        window->mouseDown(e.xbutton.x, e.xbutton.y, ui::MouseButton::Left);
                        break;
                    case 2:
                        window->mouseDown(e.xbutton.x, e.xbutton.y, ui::MouseButton::Wheel);
                        break;
                    case 3:
                        window->mouseDown(e.xbutton.x, e.xbutton.y, ui::MouseButton::Right);
                        break;
                    case 4:
                        window->mouseWheel(e.xbutton.x, e.xbutton.y, 1);
                        break;
                    case 5:
                        window->mouseWheel(e.xbutton.x, e.xbutton.y, -1);
                        break;
                    default:
                        break;
                }
                break;
            case ButtonRelease: 
				window->activeModifiers_ = ui::Key(ui::Key::Invalid, GetStateModifiers(e.xbutton.state));
				switch (e.xbutton.button) {
                    case 1:
                        window->mouseUp(e.xbutton.x, e.xbutton.y, ui::MouseButton::Left);
                        break;
                    case 2:
                        window->mouseUp(e.xbutton.x, e.xbutton.y, ui::MouseButton::Wheel);
                        break;
                    case 3:
                        window->mouseUp(e.xbutton.x, e.xbutton.y, ui::MouseButton::Right);
                        break;
                    default:
                        break;
                }
                break;
            case MotionNotify:
				window->activeModifiers_ = ui::Key(ui::Key::Invalid, GetStateModifiers(e.xbutton.state));
				window->mouseMove(e.xmotion.x, e.xmotion.y);
                break;
			/** Called when we are notified that clipboard contents is available for previously requested paste.
			
			    Get the clipboard contents and call terminal's paste event. 
			 */
			case SelectionNotify:
				if (e.xselection.property) {
					char * result;
					unsigned long resSize, resTail;
					Atom type = x11::None;
					int format = 0;
					XGetWindowProperty(window->display_, window->window_, e.xselection.property, 0, LONG_MAX / 4, False, AnyPropertyType,
						&type, &format, &resSize, &resTail, (unsigned char**)& result);
					if (type == X11Application::Instance()->clipboardIncr_)
						// buffer too large, incremental reads must be implemented
						// https://stackoverflow.com/questions/27378318/c-get-string-from-clipboard-on-linux
						NOT_IMPLEMENTED;
					else
						window->paste(std::string(result, resSize));
					XFree(result);
                 }
				 break;
			/** Called when the clipboard contents is requested by an outside app. 
			 */
			case SelectionRequest: {
				X11Application* app = X11Application::Instance();
				XSelectionEvent response;
				response.type = SelectionNotify;
				response.requestor = e.xselectionrequest.requestor;
				response.selection = e.xselectionrequest.selection;
				response.target = e.xselectionrequest.target;
				response.time = e.xselectionrequest.time;
				// by default, the request is rejected
				response.property = x11::None; 
				// if the target is TARGETS, then all supported formats should be sent, in our case this is simple, only UTF8_STRING is supported
				if (response.target == app->formatTargets_) {
					XChangeProperty(
						window->display_,
						e.xselectionrequest.requestor,
						e.xselectionrequest.property,
						e.xselectionrequest.target,
						32, // atoms are 4 bytes, so 32 bits
						PropModeReplace,
						reinterpret_cast<unsigned char const*>(&app->formatStringUTF8_),
						1
					);
					response.property = e.xselectionrequest.property;
				// otherwise, if UTF8_STRING, or a STRING is requested, we just send what we have 
				} else if (response.target == app->formatString_ || response.target == app->formatStringUTF8_) {
                    std::string clipboard = (response.selection == app->clipboardName_) ? app->clipboard_ : app->selection_;
					XChangeProperty(
						window->display_,
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
					LOG << "Error sending selection notify";
				break;
			}
			/** If we lose ownership, clear the clipboard contents with the application, or if we lose primary ownership, just clear the selection.   
			 */
			case SelectionClear: {
                LOG << "Selection clear received"; 
				X11Application* app = X11Application::Instance();
                if (e.xselectionclear.selection == app->clipboardName_)
    				app->clipboard_.clear();
                else 
                    // this makes the window to invalidate the selection, but do not emit the manual selection clear
                    window->yieldSelection();
				break;
            }
            case DestroyNotify:
                // delete the window object and remove it from the list of active windows
                delete window;
                // if it was last window, exit the terminal - safe to access unprotected by the mutex now since there are no other windows left, and UI is single threaded
                if (Windows_.empty()) {
                    throw X11Application::Terminate();
                }
                break;
			/* User-defined messages. 
			 */
			case ClientMessage:
			    if (static_cast<unsigned long>(e.xclient.data.l[0]) == X11Application::Instance()->wmDeleteMessage_) {
					ASSERT(window != nullptr) << "Attempt to destroy unknown window";
                    window->close();
				}
				break;
            default:
                break;

        }
    }


} // namespace tpp

#endif