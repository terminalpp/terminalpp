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

		TerminalWindow * createTerminalWindow(TerminalWindow::Properties const& properties, std::string const& name);



		Display* xDisplay() {
			return xDisplay_;
		}

		int xScreen() {
			return xScreen_;
		}

	protected:

		void mainLoop() override;
	private:
		friend class X11TerminalWindow;

		/** X11 display. */
	    Display* xDisplay_;
		int xScreen_;
        XIM xIm_;
		Atom clipboardFormat_;
		Atom clipboardIncr_;

	}; // tpp::Application [linux]

}

#endif