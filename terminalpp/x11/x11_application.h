#pragma once
#if (defined ARCH_UNIX && defined RENDERER_NATIVE)

#include "ui/traits/selection.h"

#include "x11.h"
#include "../application.h"

namespace tpp2 {

    class X11Application : public Application {
    public:
        static void Initialize(int argc, char ** argv) {

        }

        static X11Application * Instance() {
            return dynamic_cast<X11Application*>(Application::Instance());
        }

        void alert(std::string const & message) override ;

        void openLocalFile(std::string const & filename, bool edit) override; 

        Window * createWindow(std::string const & title, int cols, int rows) override;

        void mainLoop() override;

    private:

        friend class X11Font;
        friend class X11Window;

        X11Application();

        
		/* X11 display. */
	    Display* xDisplay_;
		int xScreen_;

    }; // X11Application

} // namespace tpp



namespace tpp {

    class X11Window;

    /** https://www.student.cs.uwaterloo.ca/~cs349/f15/resources/X/xTutorialPart1.html
     */

    class X11Application : public Application {
    public:

        static void Initialize(int argc, char ** argv) {
            MARK_AS_UNUSED(argc);
            MARK_AS_UNUSED(argv);
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

        FcConfig * fcConfig() const {
            return fcConfig_;
        }

    protected:
    
        /** Displays a GUI alert. 
         
            Because X11 does not have a simple function to display a message box, the method cheats and calls the `xmessage` command with the message as an argument which should display the message window anyways. 

            In the unlikely case that `xmessage` command is not found, the error message will be written to the stdout as a last resort. 
         */
        void alert(std::string const & message) override;

        /** Opens the given local file using the default viewer/editor. 
         
            Internally, `xdg-open` is used to determine the file to open. If edit is true, then default system editor will be launched inside the default x terminal. 

            TODO this is perhaps not the best option, but works fine-ish for now and linux default programs are a bit of a mess if tpp were to support it all. 
         */
        void openLocalFile(std::string const & filename, bool edit) override;

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

        /** Font config state. 
         */
        FcConfig * fcConfig_;

        /* Clipboard contents if the application is the owner of the clipboard selection. 
         */
        std::string clipboard_;
        std::string selection_;
        X11Window * selectionOwner_;

    }; // X11Application


} // namespace tpp


#endif