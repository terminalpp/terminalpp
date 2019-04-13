#ifdef __linux__

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>

#include "application.h"

namespace tpp {

	Application::Application() {

	}

	Application::~Application() {

	}

	void Application::mainLoop() {
		XEvent e;

	}

} // namespace tpp

#endif