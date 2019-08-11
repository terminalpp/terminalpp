#pragma once
#if (defined ARCH_UNIX)

#include "x11.h"
#include "../application.h"

namespace tpp {

    class X11Window;

    /** https://www.student.cs.uwaterloo.ca/~cs349/f15/resources/X/xTutorialPart1.html
     */

    class X11Application : public Application {
    public:

        static void Initialize() {
            new X11Application();
        }

        static X11Application * Instance() {
            return dynamic_cast<X11Application*>(Application::Instance());
        }

        Window * createWindow(std::string const & title, int cols, int rows, unsigned cellHeightPx) override;

        /** Sends given X event. 

            Because Xlib is not great with multiple threads, XFlush must be called after each event being set programatically to the queue. 
         */
        void xSendEvent(X11Window * window, XEvent & e, long mask = 0);

        void mainLoop() override;

        ~X11Application() override;

        Display* xDisplay() const {
            return xDisplay_;
        }

        int xScreen() const {
            return xScreen_;
        }

    private:
        friend class X11Window;

        X11Application();

        void openInputMethod();

		/** An exception to be thrown when the program should terminate. 
		 */
		class Terminate { };

		/* X11 display. */
	    Display* xDisplay_;
		int xScreen_;

		/* A window that always exists, is always hidden and we use it to send broadcast messages because X does not allow window-less messages and this feels simpler than copying the whole queue. 
		 */
		x11::Window broadcastWindow_;
        XIM xIm_;
		Atom wmDeleteMessage_;
		Atom fpsTimerMessage_;
        Atom primaryName_;
		Atom clipboardName_;
		Atom formatString_;
		Atom formatStringUTF8_;
		Atom formatTargets_;
		Atom clipboardIncr_;
		Atom motifWmHints_;
		Atom netWmIcon_;

        /* Clipboard contents if the application is the owner of the clipboard selection. 
         */
        std::string clipboard_;

    }; // X11Application


} // namespace tpp


#endif