#ifdef __linux__

#include "helpers/log.h"

#include "application.h"

#include "terminal_window.h"

namespace tpp {


	TerminalWindow::TerminalWindow(Application* app, TerminalSettings* settings):
		BaseTerminalWindow(settings),
	    app_(app) {
		unsigned long black = BlackPixel(app->display_, app->screen_);	/* get color black */
		unsigned long white = WhitePixel(app->display_, app->screen_);  /* get color white */
		window_ = XCreateSimpleWindow(app->display_, DefaultRootWindow(app->display_), 0, 0, 200, 300, 5, white, black);

		// from http://math.msu.su/~vvb/2course/Borisenko/CppProjects/GWindow/xintro.html

		/* here is where some properties of the window can be set.
			   The third and fourth items indicate the name which appears
			   at the top of the window and the name of the minimized window
			   respectively.
			*/
		XSetStandardProperties(app->display_, window_, "My Window", "HI!", None, NULL, 0, NULL);

		/* this routine determines which types of input are allowed in
		   the input.  see the appropriate section for details...
		*/
		XSelectInput(app->display_, window_, ExposureMask | ButtonPressMask | KeyPressMask);

		/* create the Graphics Context */
		gc_ = XCreateGC(app->display_, window_, 0, 0);

		/* here is another routine to set the foreground and background
		   colors _currently_ in use in the window.
		*/
		XSetBackground(app->display_, gc_, white);
		XSetForeground(app->display_, gc_, black);

		/* clear the window and bring it on top of the other windows */
		XClearWindow(app->display_, window_);
		XMapRaised(app_->display_, window_);
		LOG << "done";

	}

	void TerminalWindow::show() {
		XMapRaised(app_->display_, window_);
	}

	TerminalWindow::~TerminalWindow() {
		XFreeGC(app_->display_, gc_);
		XDestroyWindow(app_->display_, window_);
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