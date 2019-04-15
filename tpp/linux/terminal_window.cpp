#ifdef __linux__

#include "helpers/helpers.h"
#include "helpers/log.h"

#include "application.h"

#include "terminal_window.h"

namespace tpp {

	// http://math.msu.su/~vvb/2course/Borisenko/CppProjects/GWindow/xintro.html
	// https://keithp.com/~keithp/talks/xtc2001/paper/
    // https://en.wikibooks.org/wiki/Guide_to_X11/Fonts
	// https://keithp.com/~keithp/render/Xft.tutorial


	TerminalWindow::TerminalWindow(Application* app, TerminalSettings* settings):
		BaseTerminalWindow(settings),
	    display_(app->display_),
	    screen_(app->screen_) {
		unsigned long black = BlackPixel(display_, screen_);	/* get color black */
		unsigned long white = WhitePixel(display_, screen_);  /* get color white */
		window_ = XCreateSimpleWindow(display_, RootWindow(display_, screen_), 0, 0, 300, 300, 5, white, black);

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
		XSelectInput(display_, window_, ExposureMask | ButtonPressMask | KeyPressMask);






#ifdef HAHA

		/* create the Graphics Context */
		gc_ = XCreateGC(app->display_, window_, 0, 0);

		/* here is another routine to set the foreground and background
		   colors _currently_ in use in the window.
		*/
		XSetBackground(app->display_, gc_, white);
		XSetForeground(app->display_, gc_, black);

#endif

		/* clear the window and bring it on top of the other windows */
		XClearWindow(display_, window_);
		XMapRaised(display_, window_);

		XftFont* font = XftFontOpenName(display_, screen_, "Iosevka Term:pixelsize=16");
		XftFont* fontb = XftFontOpenName(display_, screen_, "Iosevka Term:pixelsize=16:bold");

		ASSERT(font != nullptr);

		Visual * visual = DefaultVisual(display_, screen_);
		Colormap colorMap = DefaultColormap(display_, screen_);

		XftDraw * draw = XftDrawCreate(display_, window_, visual, colorMap);

		ASSERT(draw != nullptr);

		XRenderColor c;
		c.red = 65535;
		c.green = 0;
		c.blue = 0;
		c.alpha = 65535;
		XftColor xftc;
		xftc.color = c;
		//XftColorAllocValue(app->display_, visual, colorMap, &c, &xftc);


		XftDrawString8(draw, &xftc, font, 16, 16, (XftChar8*)"Hello World!", 12);
		XftDrawString8(draw, &xftc, fontb, 16, 32, (XftChar8*)"Hello World!", 12);
		XftDrawString8(draw, &xftc, fontb, 290, 290, (XftChar8*)"Hello World!", 1);

		//XFlush(app->display_);
		LOG << "done";

	}

	void TerminalWindow::show() {
		XMapRaised(display_, window_);
	}

	TerminalWindow::~TerminalWindow() {
		//XFreeGC(display_, gc_);
		XDestroyWindow(display_, window_);
	}

	void TerminalWindow::resizeWindow(unsigned width, unsigned height) {

	}

	void TerminalWindow::repaint(vterm::Terminal::RepaintEvent& e) {

	}

	void TerminalWindow::doSetFullscreen(bool value) {

	}

	void TerminalWindow::doTitleChange(vterm::VT100::TitleEvent& e) {

	}



} // namespace tpp

#endif