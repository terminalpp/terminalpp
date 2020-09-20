#if (defined ARCH_UNIX && defined RENDERER_NATIVE)

#include "x11_window.h"

namespace tpp {
	/** The statically generated icon description stored in an array so that in can be part of the executable. 
	
	    To change its contents, run the `icons` build target. 
	 */
	extern unsigned long tppIcon[];
	extern unsigned long tppIconNotification[];

    X11Window::X11Window(std::string const & title, int cols, int rows):
        RendererWindow{cols, rows},
        focusCheck_{false},
		display_{X11Application::Instance()->xDisplay_},
		screen_{X11Application::Instance()->xScreen_},
	    visual_{DefaultVisual(display_, screen_)},
	    colorMap_{DefaultColormap(display_, screen_)},
		ic_{nullptr},
	    buffer_{0},
	    draw_{nullptr},
        text_{nullptr},
        textSize_{0} {
		unsigned long black = BlackPixel(display_, screen_);	/* get color black */
		unsigned long white = WhitePixel(display_, screen_);  /* get color white */
        x11::Window parent = XRootWindow(display_, screen_);

		window_ = XCreateSimpleWindow(display_, parent, 0, 0, sizePx_.width(), sizePx_.height(), 1, white, black);

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
		XSelectInput(display_, window_, ButtonPressMask | ButtonReleaseMask | PointerMotionMask | EnterWindowMask | LeaveWindowMask | KeyPressMask | KeyReleaseMask | StructureNotifyMask | VisibilityChangeMask | ExposureMask | FocusChangeMask);

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

        updateXftStructures(width());

		// register the window
        RegisterWindowHandle(this, window_);

        border_ = toXftColor(Color::White);

        // set the icon & title 
        setTitle(title_);
        setIcon(icon_);
    }


    X11Window::~X11Window() {
        UnregisterWindowHandle(window_);
		XFreeGC(display_, gc_);
        delete [] text_;
    }

    void X11Window::setTitle(std::string const & value) {
        RendererWindow::setTitle(value);
        XSetStandardProperties(display_, window_, value.c_str(), nullptr, x11::None, nullptr, 0, nullptr);
    }

    void X11Window::setIcon(Window::Icon icon) {
        RendererWindow::setIcon(icon);
        /* 
		    The window icon must be an array of BGRA colors for the different icon sizes where the first element is the total size of the array followed by arbitrary icon sizes encoded by first 2 items representing the icon width and height followed by the actual pixels. 
         */
        unsigned long * iconHandle;
        switch (icon) {
            case Icon::Notification:
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


    void X11Window::setFullscreen(bool value) {
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
        Window::setFullscreen(value);
    }

    void X11Window::show(bool value) {
        if (value) 
            XMapWindow(display_, window_);
        else
            NOT_IMPLEMENTED;        
    }

    void X11Window::schedule(std::function<void()> event, Widget * widget) {
        RendererWindow::schedule(event, widget);
        XEvent e;
        memset(&e, 0, sizeof(XEvent));
        e.type = ClientMessage;
        e.xclient.send_event = true;
        e.xclient.display = display_;
        e.xclient.message_type = X11Application::Instance()->xAppEvent_;
        e.xclient.format = 32;
        //e.xclient.data.l[0] = wmUserEventMessage_;
        // send the message that informs the renderer to process the queue
        X11Application::Instance()->xSendEvent(nullptr, e, NoEventMask);
    }


    void X11Window::requestClipboard(Widget * sender) {
        RendererWindow::requestClipboard(sender);
        X11Application * app = X11Application::Instance();
		XConvertSelection(display_, app->clipboardName_, app->formatStringUTF8_, app->clipboardName_, window_, CurrentTime);
    }

    void X11Window::requestSelection(Widget * sender) {
        RendererWindow::requestSelection(sender);
        X11Application * app = X11Application::Instance();
		XConvertSelection(display_, app->primaryName_, app->formatStringUTF8_, app->primaryName_, window_, CurrentTime);
    }

    void X11Window::setClipboard(std::string const & contents) {
        X11Application::Instance()->setClipboard(contents);
    }

    void X11Window::setSelection(std::string const & contents, Widget * owner) {
        MARK_AS_UNUSED(owner);
        X11Application * app = X11Application::Instance();
        X11Window * oldOwner = app->selectionOwner_;
        // set the contents in the application
		app->selection_ = contents;
		app->selectionOwner_ = this;
        // if there was a different owner before, clear its selection (no X11 event will be emited because selectionOwner is someone else already)
        if (oldOwner != nullptr && oldOwner != this)
            oldOwner->clearSelection(nullptr);
        // inform X that we own the clipboard selection
        XSetSelectionOwner(display_, app->primaryName_, window_, CurrentTime);
    }

    void X11Window::clearSelection(Widget * sender) {
        X11Application * app = X11Application::Instance();
        if (app->selectionOwner_ == this) {
            app->selectionOwner_ = nullptr;
            app->selection_.clear();
            // emit the X11 event that the selection has been cleared
            XSetSelectionOwner(display_, app->primaryName_, x11::None, CurrentTime);
        }
        // deal with the selection clear in itself
        RendererWindow::clearSelection(sender);
    }

    Key X11Window::GetStateModifiers(int state) {
        Key modifiers = Key::Invalid;
		if (state & 1)
			modifiers += Key::Shift;
		if (state & 4)
			modifiers += Key::Ctrl;
		if (state & 8)
			modifiers += Key::Alt;
		if (state & 64)
			modifiers += Key::Win;
		return modifiers;
    }

    Key X11Window::GetKey(KeySym k, Key modifiers, bool pressed) {
        if (k >= 'a' && k <= 'z') 
            return Key::FromCode(k - 32) + modifiers;
        if (k >= 'A' && k <= 'Z')
            return Key::FromCode(k) + modifiers;
        if (k >= '0' && k <= '9')
            return Key::FromCode(k) + modifiers;
        // numpad
        if (k >= XK_KP_0 && k <= XK_KP_9)
            return Key::FromCode(Key::Numpad0.code() + k - XK_KP_0) + modifiers;
        // fn keys
        if (k >= XK_F1 && k <= XK_F12)
            return Key::FromCode(Key::F1.code() + k - XK_F1) + modifiers;
        // others
        switch (k) {
            case XK_BackSpace:
                return Key::Backspace + modifiers;
            case XK_Tab:
                return Key::Tab + modifiers;
            case XK_Return:
            case XK_KP_Enter:            
                return Key::Enter + modifiers;
            case XK_Caps_Lock:
                return Key::CapsLock + modifiers;
            case XK_Escape:
                return Key::Esc + modifiers;
            case XK_space:
                return Key::Space + modifiers;
            case XK_Page_Up:
            case XK_KP_Page_Up:
                return Key::PageUp + modifiers;
            case XK_Page_Down:
            case XK_KP_Page_Down:
                return Key::PageDown + modifiers;
            case XK_End:
            case XK_KP_End:
                return Key::End + modifiers;
            case XK_Home:
            case XK_KP_Home:
                return Key::Home + modifiers;
            case XK_Left:
            case XK_KP_Left:
                return Key::Left + modifiers;
            case XK_Up:
            case XK_KP_Up:
                return Key::Up + modifiers;
            case XK_Right:
            case XK_KP_Right:
                return Key::Right + modifiers;
            case XK_Down:
            case XK_KP_Down:
                return Key::Down + modifiers;
            case XK_Insert:
            case XK_KP_Insert:
                return Key::Insert + modifiers;
            case XK_Delete:
            case XK_KP_Delete:
                return Key::Delete + modifiers;
            case XK_Menu:
                return Key::Menu + modifiers;
            case XK_KP_Multiply:
                return Key::NumpadMul + modifiers;
            case XK_KP_Add:
                return Key::NumpadAdd + modifiers;
            case XK_KP_Separator:
                return Key::NumpadComma + modifiers;
            case XK_KP_Subtract:
                return Key::NumpadSub + modifiers;
            case XK_KP_Decimal:
                return Key::NumpadDot + modifiers;
            case XK_KP_Divide:
                return Key::NumpadDiv + modifiers;
            case XK_Num_Lock:
                return Key::NumLock + modifiers;
            case XK_Scroll_Lock:
                return Key::ScrollLock + modifiers;
            case XK_semicolon: // ; and :
                return Key::Semicolon + modifiers;
            case XK_equal: // = and +
                return Key::Equals + modifiers;
            case XK_comma: // , and < 
                return Key::Comma + modifiers;
            case XK_minus: // - and _
                return Key::Minus + modifiers;
            case XK_period: // . and >
                return Key::Dot + modifiers;
            case XK_slash: // / and ?
                return Key::Slash + modifiers;
            case XK_grave: // ` and ~
                return Key::Tick + modifiers;
            case XK_bracketleft: // [ and {
                return Key::SquareOpen + modifiers;
            case XK_backslash:  // \ and |
                return Key::Backslash + modifiers;
            case XK_bracketright: // ] and }
                return Key::SquareClose + modifiers;
            case XK_apostrophe: // ' and "
                return Key::Quote + modifiers;
			case XK_Shift_L:
			case XK_Shift_R:
				if (pressed)
					modifiers += Key::Shift;
				else
					modifiers -= Key::Shift;
				return Key::ShiftKey + modifiers;
			case XK_Control_L:
			case XK_Control_R:
				if (pressed)
					modifiers += Key::Ctrl;
				else
					modifiers -= Key::Ctrl;
				return Key::CtrlKey + modifiers;
			case XK_Alt_L:
			case XK_Alt_R:
				if (pressed)
					modifiers += Key::Alt;
				else
					modifiers -= Key::Alt;
				return Key::AltKey + modifiers;
			case XK_Meta_L:
			case XK_Meta_R:
				if (pressed)
					modifiers += Key::Win;
				else
					modifiers -= Key::Win;
				return Key::WinKey + modifiers;
			default:
                return Key::Invalid;
        }
    }

    void X11Window::EventHandler(XEvent & e) {
        X11Window * window = GetWindowForHandle(e.xany.window);
        switch(e.type) {
            /* Handles repaint event when window is shown or a repaint was triggered. 
             */
            case Expose: 
                if (e.xexpose.count != 0)
                    break;
                // TODO actually get the widget to be rendered 
                window->expose();
                break;
			/** Handles when the window gets focus. 
			 */
			case FocusIn:
				if (e.xfocus.mode == NotifyGrab || e.xfocus.mode == NotifyUngrab)
					break;
				ASSERT(window != nullptr);
				window->focusIn();
				break;
			/** Handles when the window loses focus. 
			 */
			case FocusOut:
				if (e.xfocus.mode == NotifyGrab || e.xfocus.mode == NotifyUngrab)
					break;
				ASSERT(window != nullptr);
				window->focusOut();
				break;
            /* Handles window resize, which should change the terminal size accordingly. 
             */  
            case ConfigureNotify: {
                if (window->sizePx_.width() != e.xconfigure.width || window->sizePx_.height() != e.xconfigure.height)
                    window->windowResized(e.xconfigure.width, e.xconfigure.height);
                break;
            }
            /* Unlike Win32 we have to determine whether we are dealing with sendChar, or keyDown. 
             */
            case KeyPress: {
                Key modifiers = GetStateModifiers(e.xkey.state);
				window->setModifiers(modifiers);
                // see if the key corresponds to a printable characters for which keyChar should be generated
				KeySym kSym;
                char str[32];
                Status status;
				int strLen = 0;
				if (window->ic_ != nullptr)
					strLen = Xutf8LookupString(window->ic_, &e.xkey, str, sizeof str, &kSym, &status);
				else
					strLen = XLookupString(&e.xkey, str, sizeof str, &kSym, nullptr);
                // if the keysym was recognized, it is a keyDown event first
                Key key = GetKey(kSym, modifiers, true);
                // we were not able to recognize the key, but there were modifiers present, try removing them and asking for the keysymbol again
                if (key == Key::Invalid && modifiers != Key::Invalid) {
                    // clear the state
                    e.xkey.state &= ~ (1 + 4 + 8 + 64);
                    char strx[32];
                    if (window->ic_ != nullptr)
                        Xutf8LookupString(window->ic_, &e.xkey, strx, sizeof strx, &kSym, &status);
                    else
                        XLookupString(&e.xkey, strx, sizeof strx, &kSym, nullptr);
                    key = GetKey(kSym, modifiers, true);
                }
				// if the modifiers were updated (i.e. the key is Shift, Ctrl, Alt or Win, update active modifiers
				if (modifiers != key.modifiers())
                    window->setModifiers(modifiers);
				if (key != Key::Invalid)
                    window->keyDown(key);
                // if it is printable character and there were no modifiers other than shift pressed, we are dealing with printable character (backspace is not printable character)
                if (strLen > 0 && (str[0] < 0 || static_cast<unsigned char>(str[0]) >= 0x20) && (e.xkey.state & 0x4c) == 0 && static_cast<unsigned>(str[0]) != 0x7f) {
                    char const * x = pointer_cast<char const*>(& str);
                    try {
                        Char c{Char::FromUTF8(x, x + 32)};
                        window->keyChar(c);
                    } catch (CharError const &) {
                        // do nothing
                        LOG() << "error";
                    }
                }
                break;
            }
			/* The modifiers of the key correspond to the state *after* it was released. I.e. released ctrl-key will not have ctrl-modifier enabled.
			 */
            case KeyRelease: {
				Key modifiers = GetStateModifiers(e.xkey.state);
				window->setModifiers(modifiers);
				KeySym kSym = XLookupKeysym(& e.xkey, 0);
                Key key = GetKey(kSym, modifiers, false);
				// if the modifiers were updated (i.e. the key is Shift, Ctrl, Alt or Win, updated active modifiers
				if (modifiers != key.modifiers())
					window->setModifiers(modifiers);
				if (key != Key::Invalid)
                    window->keyUp(key);
                break;
            }
            case ButtonPress: 
                // TODO is it really necessary to update the modifiers here?
				window->setModifiers(GetStateModifiers(e.xbutton.state));
				switch (e.xbutton.button) {
                    case 1:
                        window->mouseDown(window->pixelsToCoords(Point{e.xbutton.x, e.xbutton.y}), MouseButton::Left);
                        break;
                    case 2:
                        window->mouseDown(window->pixelsToCoords(Point{e.xbutton.x, e.xbutton.y}), MouseButton::Wheel);
                        break;
                    case 3:
                        window->mouseDown(window->pixelsToCoords(Point{e.xbutton.x, e.xbutton.y}), MouseButton::Right);
                        break;
                    case 4:
                        window->mouseWheel(window->pixelsToCoords(Point{e.xbutton.x, e.xbutton.y}), 1);
                        break;
                    case 5:
                        window->mouseWheel(window->pixelsToCoords(Point{e.xbutton.x, e.xbutton.y}), -1);
                        break;
                    default:
                        break;
                }
                break;
            case ButtonRelease: 
                // TODO is it really necessary to update the modifiers here?
				window->setModifiers(GetStateModifiers(e.xbutton.state));
				switch (e.xbutton.button) {
                    case 1:
                        window->mouseUp(window->pixelsToCoords(Point{e.xbutton.x, e.xbutton.y}), MouseButton::Left);
                        break;
                    case 2:
                        window->mouseUp(window->pixelsToCoords(Point{e.xbutton.x, e.xbutton.y}), MouseButton::Wheel);
                        break;
                    case 3:
                        window->mouseUp(window->pixelsToCoords(Point{e.xbutton.x, e.xbutton.y}), MouseButton::Right);
                        break;
                    default:
                        break;
                }
                break;
            case MotionNotify:
                // TODO is it really necessary to update the modifiers here?
				window->setModifiers(GetStateModifiers(e.xbutton.state));
				window->mouseMove(window->pixelsToCoords(Point{e.xmotion.x, e.xmotion.y}));
                break;
            /* Mouse enters the window. 
             */
            case EnterNotify:
                window->mouseIn();
                break;
            /* Mouse leaves the window. 
             */
            case LeaveNotify:
                window->mouseOut();
                break;
			/** Called when we are notified that clipboard or selection contents is available for previously requested paste.
             
                 SelectionCLear and SelectionRequest are handled by the Application.
			 */
			case SelectionNotify:
				if (e.xselection.property) {
    				X11Application* app = X11Application::Instance();
					char * result;
					unsigned long resSize, resTail;
					Atom type = x11::None;
					int format = 0;
					XGetWindowProperty(window->display_, window->window_, e.xselection.property, 0, LONG_MAX / 4, False, AnyPropertyType,
						&type, &format, &resSize, &resTail, (unsigned char**)& result);
					if (type == X11Application::Instance()->clipboardIncr_) {
						// buffer too large, incremental reads must be implemented
						// https://stackoverflow.com/questions/27378318/c-get-string-from-clipboard-on-linux
						NOT_IMPLEMENTED;
                    } else {
                        if (e.xselection.selection == app->clipboardName_)
						    window->pasteClipboard(std::string(result, resSize));
                        else if (e.xselection.selection == app->primaryName_)
						    window->pasteSelection(std::string(result, resSize));
                    }
					XFree(result);
                 }
				 break;
            case DestroyNotify: {
                // delete the window object and remove it from the list of active windows
                delete window;
                if (Windows().empty()) 
                    throw X11Application::TerminateException();
                break;
            }

			/* User-defined messages. 
			 */
			case ClientMessage:
			    if (static_cast<unsigned long>(e.xclient.data.l[0]) == X11Application::Instance()->wmDeleteMessage_) {
					ASSERT(window != nullptr) << "Attempt to destroy unknown window";
                    window->requestClose();
				}
				break;
        }
    }

} // namespace tpp

#endif