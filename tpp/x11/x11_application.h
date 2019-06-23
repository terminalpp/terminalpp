#pragma once
#ifdef __linux__


#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xft/Xft.h>

#include "../application.h"

namespace tpp {

	class X11TerminalWindow;

    /** https://www.student.cs.uwaterloo.ca/~cs349/f15/resources/X/xTutorialPart1.html
     */
	class X11Application : public Application {
	public:
		X11Application();
		~X11Application() override;

		TerminalWindow * createTerminalWindow(Session * session, TerminalWindow::Properties const& properties, std::string const& name);



        /** Sends given X event. 

            Because Xlib is not great with multiple threads, XFlush must be called after each event being set programatically to the queue. 
         */
        void xSendEvent(X11TerminalWindow * window, XEvent & e, long mask = 0);

		Display* xDisplay() {
			return xDisplay_;
		}

		int xScreen() {
			return xScreen_;
		}

	protected:

		void mainLoop() override;

		void sendBlinkTimerMessage() override {
			XEvent e;
			memset(&e, 0, sizeof(XEvent));
			e.xclient.type = ClientMessage;
			e.xclient.display = xDisplay_;
			e.xclient.window = None;
			e.xclient.format = 32;
			e.xclient.data.l[0] = blinkTimerMessage_;
			xSendEvent(nullptr, e);
		}
	private:
		friend class X11TerminalWindow;

		class Terminate { };

		/** X11 display. */
	    Display* xDisplay_;
		int xScreen_;
		/** A window that always exists, is always hidden and we use it to send broadcast messages because X does not allow window-less messages and this feels simpler than copying the whole queue. 
		 */
		Window broadcastWindow_;
        XIM xIm_;
		Atom wmDeleteMessage_;
		Atom inputReadyMessage_;
		Atom blinkTimerMessage_;
		Atom clipboardName_;
		Atom formatString_;
		Atom formatStringUTF8_;
		Atom formatTargets_;
		Atom clipboardIncr_;
		std::string clipboard_;

	}; // tpp::Application [linux]

}

#endif
