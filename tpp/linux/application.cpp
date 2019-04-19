#ifdef __linux__

#include "helpers/helpers.h"
#include "helpers/log.h"

#include "application.h"
#include "terminal_window.h"

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
            if (XFilterEvent(&e, None))
                continue;
            TerminalWindow::EventHandler(e);
       }
	}

} // namespace tpp

#endif