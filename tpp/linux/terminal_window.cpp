#ifdef __linux__

#include "helpers/helpers.h"
#include "helpers/log.h"

#include "application.h"

#include "terminal_window.h"

namespace tpp {

    std::unordered_map<Window, TerminalWindow *> TerminalWindow::Windows_;


	// http://math.msu.su/~vvb/2course/Borisenko/CppProjects/GWindow/xintro.html
	// https://keithp.com/~keithp/talks/xtc2001/paper/
    // https://en.wikibooks.org/wiki/Guide_to_X11/Fonts
	// https://keithp.com/~keithp/render/Xft.tutorial


	TerminalWindow::TerminalWindow(Application* app, TerminalSettings* settings) :
		BaseTerminalWindow(settings),
		display_(Application::XDisplay()),
		screen_(Application::XScreen()),
	    visual_(DefaultVisual(display_, screen_)),
	    colorMap_(DefaultColormap(display_, screen_)),
	    buffer_(0),
	    draw_(nullptr) {
		unsigned long black = BlackPixel(display_, screen_);	/* get color black */
		unsigned long white = WhitePixel(display_, screen_);  /* get color white */
        Window parent = RootWindow(display_, screen_);

		window_ = XCreateSimpleWindow(display_, parent, 0, 0, widthPx_, heightPx_, 1, white, black);

		// from http://math.msu.su/~vvb/2course/Borisenko/CppProjects/GWindow/xintro.html

		/* here is where some properties of the window can be set.
			   The third and fourth items indicate the name which appears
			   at the top of the window and the name of the minimized window
			   respectively.
			*/
		XSetStandardProperties(display_, window_, title_.c_str(), nullptr, None, nullptr, 0, nullptr);

		/* this routine determines which types of input are allowed in
		   the input.  see the appropriate section for details...
		*/
		XSelectInput(display_, window_, ButtonPressMask | ButtonReleaseMask | PointerMotionMask | KeyPressMask | KeyReleaseMask | StructureNotifyMask | VisibilityChangeMask | ExposureMask | FocusChangeMask);

        XGCValues gcv;
        memset(&gcv, 0, sizeof(XGCValues));
    	gcv.graphics_exposures = False;
        gc_ = XCreateGC(display_, parent, GCGraphicsExposures, &gcv);

        // create input context for the window... The extra arguments to the XCreateIC are c-c c-v from the internet and for now are a mystery to me
    	ic_ = XCreateIC(Application::XIm_, XNInputStyle, XIMPreeditNothing | XIMStatusNothing,
    				XNClientWindow, window_, XNFocusWindow, window_, nullptr);

        Windows_[window_] = this;
	}

	void TerminalWindow::show() {
        XMapWindow(display_, window_);
		//XMapRaised(display_, window_);
	}

	TerminalWindow::~TerminalWindow() {
        Windows_.erase(window_);
		doInvalidate();
		XFreeGC(display_, gc_);
		XDestroyWindow(display_, window_);
	}

	void TerminalWindow::doSetFullscreen(bool value) {
        // TODO Implement
	}

	void TerminalWindow::titleChange(vterm::Terminal::TitleChangeEvent & e) {
        // TODO implement
	}

	void TerminalWindow::doPaint() {
        std::lock_guard<std::mutex> g(drawGuard_);
		ASSERT(draw_ == nullptr);
		bool forceDirty = false;
        if (invalidated_ && buffer_ != 0) {
            XFreePixmap(display_, buffer_);
            buffer_ = 0;
        }
		if (buffer_ == 0) {
			buffer_ = XCreatePixmap(display_, window_, widthPx_, heightPx_, DefaultDepth(display_, screen_));
			ASSERT(buffer_ != 0);
			forceDirty = true;
            invalidated_ = false;
		}
		draw_ = XftDrawCreate(display_, buffer_, visual_, colorMap_);
		doUpdateBuffer(forceDirty);
        // first clear the borders that won't be used (don't clear the whole window to prevent flicker)
        unsigned marginRight = widthPx_ % cellWidthPx_;
        unsigned marginBottom = heightPx_ % cellHeightPx_;
        if (marginRight != 0) 
            XClearArea(display_, window_, widthPx_ - marginRight, 0, marginRight, heightPx_, false);
        if (marginBottom != 0)
            XClearArea(display_, window_, 0, heightPx_ - marginBottom, widthPx_, marginBottom, false);
        // now bitblt the buffer
		XCopyArea(display_, buffer_, window_, gc_, 0, 0, widthPx_, heightPx_, 0, 0);
		XftDrawDestroy(draw_);
		draw_ = nullptr;
        XFlush(display_);
	}

    vterm::Key TerminalWindow::GetKey(KeySym k, unsigned state) {
        unsigned modifiers = 0;
        if (state & 1)
            modifiers += vterm::Key::Shift;
        if (state & 4)
            modifiers += vterm::Key::Ctrl;
        if (state & 8)
            modifiers += vterm::Key::Alt;
        if (state & 64)
            modifiers += vterm::Key::Win;
        if (k >= 'a' && k <= 'z') 
            return vterm::Key(k - 32, modifiers);
        if (k >= 'A' && k <= 'Z')
            return vterm::Key(k, modifiers);
        if (k >= '0' && k <= '9')
            return vterm::Key(k, modifiers);
        // numpad
        if (k >= XK_0 && k <= XK_9)
            return vterm::Key(vterm::Key::Numpad0 + k - XK_0, modifiers);
        // fn keys
        if (k >= XK_F1 && k <= XK_F12)
            return vterm::Key(vterm::Key::F1 + k - XK_F1, modifiers);
        // others
        switch (k) {
            case XK_BackSpace:
                return vterm::Key(vterm::Key::Backspace, modifiers);
            case XK_Tab:
                return vterm::Key(vterm::Key::Tab, modifiers);
            case XK_Return:
                return vterm::Key(vterm::Key::Enter, modifiers);
            case XK_Caps_Lock:
                return vterm::Key(vterm::Key::CapsLock, modifiers);
            case XK_Escape:
                return vterm::Key(vterm::Key::Esc, modifiers);
            case XK_space:
                return vterm::Key(vterm::Key::Space, modifiers);
            case XK_Page_Up:
                return vterm::Key(vterm::Key::PageUp, modifiers);
            case XK_Page_Down:
                return vterm::Key(vterm::Key::PageDown, modifiers);
            case XK_End:
                return vterm::Key(vterm::Key::End, modifiers);
            case XK_Home:
                return vterm::Key(vterm::Key::Home, modifiers);
            case XK_Left:
                return vterm::Key(vterm::Key::Left, modifiers);
            case XK_Up:
                return vterm::Key(vterm::Key::Up, modifiers);
            case XK_Right:
                return vterm::Key(vterm::Key::Right, modifiers);
            case XK_Down:
                return vterm::Key(vterm::Key::Down, modifiers);
            case XK_Insert:
                return vterm::Key(vterm::Key::Insert, modifiers);
            case XK_Delete:
                return vterm::Key(vterm::Key::Delete, modifiers);
            case XK_Menu:
                return vterm::Key(vterm::Key::Menu, modifiers);
            case XK_KP_Multiply:
                return vterm::Key(vterm::Key::NumpadMul, modifiers);
            case XK_KP_Add:
                return vterm::Key(vterm::Key::NumpadAdd, modifiers);
            case XK_KP_Separator:
                return vterm::Key(vterm::Key::NumpadComma, modifiers);
            case XK_KP_Subtract:
                return vterm::Key(vterm::Key::NumpadSub, modifiers);
            case XK_KP_Decimal:
                return vterm::Key(vterm::Key::NumpadDot, modifiers);
            case XK_KP_Divide:
                return vterm::Key(vterm::Key::NumpadDiv, modifiers);
            case XK_Num_Lock:
                return vterm::Key(vterm::Key::NumLock, modifiers);
            case XK_Scroll_Lock:
                return vterm::Key(vterm::Key::ScrollLock, modifiers);
            case XK_semicolon:
                return vterm::Key(vterm::Key::Semicolon, modifiers);
            case XK_equal:
                return vterm::Key(vterm::Key::Equals, modifiers);
            case XK_comma:
                return vterm::Key(vterm::Key::Comma, modifiers);
            case XK_minus:
                return vterm::Key(vterm::Key::Minus, modifiers);
            case XK_period: // .
                return vterm::Key(vterm::Key::Dot, modifiers);
            case XK_slash:
                return vterm::Key(vterm::Key::Slash, modifiers);
            case XK_grave: // `
                return vterm::Key(vterm::Key::Tick, modifiers);
            case XK_bracketleft: // [
                return vterm::Key(vterm::Key::SquareOpen, modifiers);
            case XK_backslash:  
                return vterm::Key(vterm::Key::Backslash, modifiers);
            case XK_bracketright: // ]
                return vterm::Key(vterm::Key::SquareClose, modifiers);
            case XK_apostrophe: // '
                return vterm::Key(vterm::Key::Quote, modifiers);
            default:
                return vterm::Key(vterm::Key::Invalid, 0);
        }
    }

    void TerminalWindow::EventHandler(XEvent & e) {
        TerminalWindow * tw = nullptr;
        auto i = Windows_.find(e.xany.window);
        if (i != Windows_.end())
            tw = i->second;
        switch(e.type) {
            /* Handles repaint event when window is shown or a repaint was triggered. 
             */
            case Expose: 
                tw->doPaint();
                break;
            /* Handles window resize, which should change the terminal size accordingly. 
             */  
            case ConfigureNotify: {
                if (tw->widthPx_ != static_cast<unsigned>(e.xconfigure.width) || tw->heightPx_ != static_cast<unsigned>(e.xconfigure.height))
                    tw->resizeWindow(e.xconfigure.width, e.xconfigure.height);
                break;
            }
            case MapNotify:
                break;
            /* Unlike Win32 we have to determine whether we are dealing with sendChar, or keyDown. 
             */
            case KeyPress: {
                KeySym kSym;
                char str[32];
                Status status;
                int strLen = Xutf8LookupString(tw->ic_, & e.xkey, str, sizeof str, &kSym, &status);
                // if it is printable character and there were no modifiers other than shift pressed, we are dealing with printable character
                if (strLen > 0 && str[0] >= 0x20 && (e.xkey.state & 0x4c) == 0) {
                    vterm::Char::UTF8 c;
                    char * x = reinterpret_cast<char*>(& str);
                    if (c.readFromStream(x, x+32)) {
                        tw->keyChar(c);
                        break;
                    }
                }
                // otherwise if the keysym was recognized, it is a keyDown event
                vterm::Key key = GetKey(kSym, e.xkey.state);
                if (key != vterm::Key::Invalid)
                    tw->keyDown(key);
                break;
            }
            case KeyRelease: {
                KeySym kSym = XLookupKeysym(& e.xkey, 0);
                vterm::Key key = GetKey(kSym, e.xkey.state);
                if (key != vterm::Key::Invalid)
                    tw->keyUp(key);
                break;
            }
            case ButtonPress: 
                switch (e.xbutton.button) {
                    case 1:
                        tw->mouseDown(e.xbutton.x, e.xbutton.y, vterm::MouseButton::Left);
                        break;
                    case 2:
                        tw->mouseDown(e.xbutton.x, e.xbutton.y, vterm::MouseButton::Wheel);
                        break;
                    case 3:
                        tw->mouseDown(e.xbutton.x, e.xbutton.y, vterm::MouseButton::Right);
                        break;
                    case 4:
                        tw->mouseWheel(e.xbutton.x, e.xbutton.y, 1);
                        break;
                    case 5:
                        tw->mouseWheel(e.xbutton.x, e.xbutton.y, -1);
                        break;
                    default:
                        break;
                }
                break;
            case ButtonRelease: 
                switch (e.xbutton.button) {
                    case 1:
                        tw->mouseUp(e.xbutton.x, e.xbutton.y, vterm::MouseButton::Left);
                        break;
                    case 2:
                        tw->mouseUp(e.xbutton.x, e.xbutton.y, vterm::MouseButton::Wheel);
                        break;
                    case 3:
                        tw->mouseUp(e.xbutton.x, e.xbutton.y, vterm::MouseButton::Right);
                        break;
                    default:
                        break;
                }
                break;
            case MotionNotify:
                tw->mouseMove(e.xmotion.x, e.xmotion.y);
                break;
            default:
                break;
        }
    }

} // namespace tpp

#endif