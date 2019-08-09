#pragma once
#if (defined ARCH_UNIX)

#include "x11.h"
#include "../application.h"

namespace tpp {

    class X11Window

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

    private:
        friend class X11Window;
		friend class Font<XftFont*>;

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
		Window broadcastWindow_;
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

    }; // X11Application


} // namespace tpp


#endif