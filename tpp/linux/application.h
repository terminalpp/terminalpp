#pragma once
#ifdef __linux__

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xft/Xft.h>

#include "../base_application.h"

namespace tpp {

	class Application : public BaseApplication {
	public:
		Application();
		~Application() override;

		void mainLoop() override;

	private:
		friend class TerminalWindow;

		/** X11 display. */
		Display* display_;
		int screen_;

	}; // tpp::Application [linux]

}

#endif