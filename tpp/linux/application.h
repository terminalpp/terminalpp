#pragma once
#ifdef __linux__

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xft/Xft.h>

#include "../base_application.h"

namespace tpp {

	class TerminalWindow;

	class Application : public BaseApplication {
	public:
		Application();
		~Application() override;

		void mainLoop() override;

		// Can't be named display because there is already a class named display in X11
		static Display* XDisplay() {
			return XDisplay_;
		}

		static int XScreen() {
			return XScreen_;
		}

	private:
		friend class TerminalWindow;

		/** X11 display. */
		static Display* XDisplay_;
		static int XScreen_;
        static XIM XIm_;

	}; // tpp::Application [linux]

}

#endif