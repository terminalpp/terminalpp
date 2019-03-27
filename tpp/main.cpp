#include <iostream>

#ifdef WIN32
#include "win32/pty_terminal.h"
#include "win32/application.h"
#include "win32/terminal_window.h"
// link to directwrite
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib") 
#else
#error "Unsupported platform"
#endif

using namespace tpp;

// https://www.codeguru.com/cpp/misc/misc/graphics/article.php/c16139/Introduction-to-DirectWrite.htm

// https://docs.microsoft.com/en-us/windows/desktop/gdi/windows-gdi

// https://docs.microsoft.com/en-us/windows/desktop/api/_gdi/


/** Terminal++ App Entry Point

    For now creates single terminal window and one virtual terminal. 
 */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {


	TerminalSettings ts;
	ts.defaultCols = 80;
	ts.defaultRows = 25;
	ts.defaultName = "terminal++";
	ts.defaultFontHeight = 16;
	ts.defaultFontWidth = 10;
	ts.defaultZoom = 1;
	ts.font = "Iosevka NF";

	Application * app = new Application(hInstance);
	std::cout << "OH HAI, CAN I HAZ CONSOLE?" << std::endl;
	TerminalWindow * tw = new TerminalWindow(app, &ts);
	tw->show();


	//vterm::ConPTYTerminal * t(new vterm::ConPTYTerminal("wsl -e echo hello", 80, 25));
	Terminal * t = new Terminal("wsl -e echo hello", 80, 25);
	t->execute();

	// create the application
	//application = new GDIApplication(hInstance);

	// create the screen buffer
	//vterm::VT100 * vterm1 = new vterm::VT100(80,25);



	// and create the terminal window
	//TerminalWindow * tw = application->createNewTerminalWindow();
	//tw->setTerminal(vterm1);
	//tw->show();

	//ConPTYProcess c("wsl -e echo hello", vterm1);
	//ConPTYConnector c("wsl -e ls /home/peta/devel -la", vterm1);
	//ConPTYConnector c("wsl -e ping www.seznam.cz", vterm1);
	//ConPTYConnector c("wsl -e mc", vterm1);
	//ConPTYConnector c("wsl -e screenfetch", vterm1);
	// Now run the main loop
	//application->mainLoop();
	app->mainLoop();

	return EXIT_SUCCESS;
}
