#include <iostream>

#include "helpers/log.h"


#ifdef WIN32
#include "win32/pty_terminal.h"
#include "win32/application.h"
#include "win32/terminal_window.h"
// link to directwrite
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#elif __linux__
#include "linux/pty_terminal.h"
#include "linux/application.h"
#include "linux/terminal_window.h"
#else
#error "Unsupported platform"
#endif



using namespace tpp;

// https://www.codeguru.com/cpp/misc/misc/graphics/article.php/c16139/Introduction-to-DirectWrite.htm

// https://docs.microsoft.com/en-us/windows/desktop/gdi/windows-gdi

// https://docs.microsoft.com/en-us/windows/desktop/api/_gdi/

// https://github.com/Microsoft/node-pty/blob/master/src/win/conpty.cc


void FixMissingSettings(TerminalSettings & ts) {
	vterm::Font defaultFont;
	TerminalWindow::Font* f = TerminalWindow::Font::GetOrCreate(defaultFont, ts.defaultFontHeight, 1);
	ts.defaultFontWidth = f->widthPx();
}



/** Terminal++ App Entry Point

    For now creates single terminal window and one virtual terminal. 
 */
#ifdef WIN32

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {

	Application* app = new Application(hInstance);

	TerminalSettings ts;
	ts.defaultCols = 80;
	ts.defaultRows = 25;
	ts.defaultFontHeight = 18;
	ts.defaultFontWidth = 0;
	ts.defaultZoom = 1;

	FixMissingSettings(ts);

	std::cout << "OH HAI, CAN I HAZ CONSOLE?" << std::endl;


	// initialize log levels we use
	//helpers::Log::RegisterLogger((new helpers::StreamLogger(vterm::VT100::SEQ, std::cout))->toFile("c:/delete/tpp.txt"));
	helpers::Log::RegisterLogger((new helpers::StreamLogger(vterm::VT100::UNKNOWN_SEQ, std::cout)));

	TerminalWindow * tw = new TerminalWindow(app, &ts);
	tw->show();


	//Terminal * t = new Terminal("wsl -e echo hello mmoo", 80, 25);
	//Terminal * t = new Terminal("wsl -e ping www.seznam.cz", 80, 25);
	//Terminal * t = new Terminal("wsl -e screenfetch", 80, 25, vterm::Palette::Colors16, 15, 0);
	Terminal * t = new Terminal("wsl -e bash", 80, 25, vterm::Palette::Colors16, 15, 0);
    //Terminal * t = new Terminal("wsl -e emacs ~/settings/emacs/init.el", 80, 25, vterm::Palette::Colors16, 15, 0);
	//Terminal * t = new Terminal("wsl -e bash -c \"ssh orange \"", 80, 25, vterm::Palette::Colors16, 15, 0);

	t->execute();

	tw->attachTerminal(t);

	app->mainLoop();

	return EXIT_SUCCESS;
}

#elif __linux__

#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
 /*
int main(void) {
   Display *d;
   Window w;
   XEvent e;
   const char *msg = "Hello, World!";
   int s;
 
   d = XOpenDisplay(NULL);
   if (d == NULL) {
      fprintf(stderr, "Cannot open display\n");
      exit(1);
   }
 
   s = DefaultScreen(d);
   w = XCreateSimpleWindow(d, RootWindow(d, s), 10, 10, 100, 100, 1,
                           BlackPixel(d, s), WhitePixel(d, s));
   XSelectInput(d, w, ExposureMask | KeyPressMask);
   XMapWindow(d, w);
 
   while (1) {
      XNextEvent(d, &e);
      if (e.type == Expose) {
         XFillRectangle(d, w, DefaultGC(d, s), 20, 20, 10, 10);
         XDrawString(d, w, DefaultGC(d, s), 10, 50, msg, strlen(msg));
      }
      if (e.type == KeyPress)
         break;
   }
 
   XCloseDisplay(d);
   return 0;
} */

int main(int argc, char* argv[]) {

	Application* app = new Application();

	TerminalSettings ts;
	ts.defaultCols = 80;
	ts.defaultRows = 25;
	ts.defaultFontHeight = 18;
	ts.defaultFontWidth = 0;
	ts.defaultZoom = 1;

	FixMissingSettings(ts);


	TerminalWindow* tw = new TerminalWindow(app, &ts);
	tw->show();

	Terminal* t = new Terminal("wsl -e bash", 80, 25, vterm::Palette::Colors16, 15, 0);

	tw->attachTerminal(t);


	app->mainLoop();

	return EXIT_SUCCESS;
} 

#endif
