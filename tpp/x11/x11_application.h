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
	private:
		friend class X11TerminalWindow;

		class Terminate { };

		/** X11 display. */
	    Display* xDisplay_;
		int xScreen_;
        XIM xIm_;
		Atom wmDeleteMessage_;
		Atom inputReadyMessage_;
		Atom clipboardName_;
		Atom formatString_;
		Atom formatStringUTF8_;
		Atom formatTargets_;
		Atom clipboardIncr_;
		std::string clipboard_;

	}; // tpp::Application [linux]

}

#endif

#ifdef HAHA

XSelectionRequestEvent* xsre;
568 	XSelectionEvent xev;
569 	Atom xa_targets, string, clipboard;
570 	char* seltext;
571
572 	xsre = (XSelectionRequestEvent*)e;
573 	xev.type = SelectionNotify;
574 	xev.requestor = xsre->requestor;
575 	xev.selection = xsre->selection;
576 	xev.target = xsre->target;
577 	xev.time = xsre->time;
578 	if (xsre->property == None)
579 		xsre->property = xsre->target;
580
581 	/* reject */
582 	xev.property = None;
583
584 	xa_targets = XInternAtom(xw.dpy, "TARGETS", 0);
585 	if (xsre->target == xa_targets) {
	586 		/* respond with the supported type */
		587 		string = xsel.xtarget;
	588 		XChangeProperty(xsre->display, xsre->requestor, xsre->property,
		589 				XA_ATOM, 32, PropModeReplace,
		590 				(uchar*)& string, 1);
	591 		xev.property = xsre->property;
	592
}
else if (xsre->target == xsel.xtarget || xsre->target == XA_STRING) {
	593 		/*
	594 		 * xith XA_STRING non ascii characters may be incorrect in the
	595 		 * requestor. It is not our problem, use utf8.
	596 		 */
		597 		clipboard = XInternAtom(xw.dpy, "CLIPBOARD", 0);
	598 		if (xsre->selection == XA_PRIMARY) {
		599 			seltext = xsel.primary;
		600
	}
	else if (xsre->selection == clipboard) {
		601 			seltext = xsel.clipboard;
		602
	}
	else {
		603 			fprintf(stderr,
			604 				"Unhandled clipboard selection 0x%lx\n",
			605 				xsre->selection);
		606 			return;
		607
	}
	608 		if (seltext != NULL) {
		609 			XChangeProperty(xsre->display, xsre->requestor,
			610 					xsre->property, xsre->target,
			611 					8, PropModeReplace,
			612 					(uchar*)seltext, strlen(seltext));
		613 			xev.property = xsre->property;
		614
	}
	615
}
616
617 	/* all done, send a notification to the listener */
618 	if (!XSendEvent(xsre->display, xsre->requestor, 1, 0, (XEvent*)& xev))
619 		fprintf(stderr, "Error sending SelectionNotify event\n");

#endif