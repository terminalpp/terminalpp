#include <iostream>

#include "helpers/log.h"


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
	ts.defaultFontHeight = 18;
	ts.defaultFontWidth = 0;
	ts.defaultZoom = 1;

	Application * app = new Application(hInstance);
	std::cout << "OH HAI, CAN I HAZ CONSOLE?" << std::endl;
	// initialize log levels we use
	helpers::Log::RegisterLogger((new helpers::StreamLogger("VT100", std::cout))->toFile("c:/delete/tpp.txt"));
	helpers::Log::RegisterLogger((new helpers::StreamLogger("VT100_Unknown", std::cout)));

	TerminalWindow * tw = new TerminalWindow(app, &ts);
	tw->show();


	//Terminal * t = new Terminal("wsl -e echo hello mmoo", 80, 25);
	//Terminal * t = new Terminal("wsl -e ping www.seznam.cz", 80, 25);
	//Terminal * t = new Terminal("wsl -e screenfetch", 80, 25, vterm::Palette::Colors16, 15, 0);
	Terminal * t = new Terminal("wsl -e mc", 80, 25, vterm::Palette::Colors16, 15, 0);

	t->execute();

	tw->attachTerminal(t);

	app->mainLoop();

	return EXIT_SUCCESS;
}
