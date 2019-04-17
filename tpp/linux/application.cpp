#ifdef __linux__

#include "helpers/helpers.h"

#include "application.h"

namespace tpp {

	Display* Application::XDisplay_ = nullptr;
	int Application::XScreen_ = 0;

	Application::Application() {
		ASSERT(XDisplay_ == nullptr) << "Application is a singleton";
		XDisplay_ = XOpenDisplay(nullptr);
		if (XDisplay_ == nullptr)
			THROW(helpers::Exception("Unable to open X display"));
		XScreen_ = DefaultScreen(XDisplay_);
	}

	Application::~Application() {
		XCloseDisplay(XDisplay_);
		XDisplay_ = nullptr;
	}

	void Application::mainLoop() {
		XEvent e;
		while (true) {
			XNextEvent(XDisplay_, &e);
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