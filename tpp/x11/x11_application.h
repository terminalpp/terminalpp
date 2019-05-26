#pragma once
#ifdef __linux__

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xft/Xft.h>

#include "../application.h"

namespace tpp {

	class X11TerminalWindow;

	class X11Application : public Application {
	public:
		X11Application();
		~X11Application() override;

		void mainLoop() override;

		// Can't be named display because there is already a class named display in X11
		static Display* XDisplay() {
			return XDisplay_;
		}

		static int XScreen() {
			return XScreen_;
		}

	private:
		friend class X11TerminalWindow;

		/** X11 display. */
		static Display* XDisplay_;
		static int XScreen_;
        static XIM XIm_;
		static Atom ClipboardFormat_;
		static Atom ClipboardIncr_;

	}; // tpp::Application [linux]

}

#endif