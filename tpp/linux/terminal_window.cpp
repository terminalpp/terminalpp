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

		LOG << widthPx_ << " x " << heightPx_;

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
		XSelectInput(display_, window_, ButtonPressMask | ButtonReleaseMask | ButtonMotionMask | KeyPressMask | KeyReleaseMask | StructureNotifyMask | VisibilityChangeMask | ExposureMask | FocusChangeMask);

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

	}

	void TerminalWindow::doTitleChange(vterm::VT100::TitleEvent& e) {

	}

	void TerminalWindow::doPaint() {
        std::lock_guard<std::mutex> g(drawGuard_);
		LOG << "doPaint";
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
		LOG << "doPaint done";
	}

    void TerminalWindow::EventHandler(XEvent & e) {
        TerminalWindow * tw = nullptr;
        auto i = Windows_.find(e.xany.window);
        if (i != Windows_.end())
            tw = i->second;
        switch(e.type) {
            case Expose: 
                LOG << "Exposed";
                tw->doPaint();
                break;
            /** Handles window resize, which should change the terminal size accordingly. 
             */  
            case ConfigureNotify: {
                if (tw->widthPx_ != static_cast<unsigned>(e.xconfigure.width) || tw->heightPx_ != static_cast<unsigned>(e.xconfigure.height))
                    tw->resizeWindow(e.xconfigure.width, e.xconfigure.height);
                break;
            }
            case MapNotify:
                break;
            case KeyPress: {
                KeySym kSym;
                char str[32];
                Status status;
                int strLen = Xutf8LookupString(tw->ic_, & e.xkey, str, sizeof str, &kSym, &status);
                //int strLen = XLookupString(&e.xkey, str, sizeof(str), &kSym, nullptr);
                str[strLen] = 0;
                LOG << "Key press " << XKeysymToString(kSym) << "(" << kSym << "):" <<  str << "(size: " << strLen << ")";
				tw->redraw();
                return;
            }
            case ButtonPress:
                break;
            default:
                break;
        }
    }

} // namespace tpp

#endif