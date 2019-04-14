#ifdef __linux__

#include "application.h"

namespace tpp {

	Application::Application():
	    display_(XOpenDisplay(nullptr)),
		screen_(DefaultScreen(display_)) {
	}

	Application::~Application() {
		XCloseDisplay(display_);
	}

	void Application::mainLoop() {
		XEvent e;
		while (true) {
			XNextEvent(display_, &e);
			if (e.type == Expose) {
				//XFillRectangle(d, w, DefaultGC(d, s), 20, 20, 10, 10);
				//XDrawString(d, w, DefaultGC(d, s), 10, 50, msg, strlen(msg));
			}
			if (e.type == KeyPress)
				break;
       }
	}

} // namespace tpp

#endif