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

		}

	}

} // namespace tpp

#endif