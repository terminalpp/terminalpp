#include <iostream>


#include "vterm/vt100.h"

#include "tpp.h"
#include "conpty/conpty_process.h"


using namespace tpp;


void AttachConsole() {
	if (AllocConsole() == 0)
		THROW(Win32Error("Cannot allocate console"));
	// this is ok, console cannot be detached, so we are fine with keeping the file handles forewer,
	// nor do we need to FreeConsole at any point
	FILE *fpstdin = stdin, *fpstdout = stdout, *fpstderr = stderr;
	// patch the cin, cout, cerr
	freopen_s(&fpstdin, "CONIN$", "r", stdin);
	freopen_s(&fpstdout, "CONOUT$", "w", stdout);
	freopen_s(&fpstderr, "CONOUT$", "w", stderr);
	std::cin.clear();
	std::cout.clear();
	std::cerr.clear();
}


// https://www.codeguru.com/cpp/misc/misc/graphics/article.php/c16139/Introduction-to-DirectWrite.htm

// https://docs.microsoft.com/en-us/windows/desktop/gdi/windows-gdi

// https://docs.microsoft.com/en-us/windows/desktop/api/_gdi/

/** Terminal++ App Entry Point

    For now creates single terminal window and one virtual terminal. 
 */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {


	AttachConsole();
	std::cout << "OH HAI, CAN I HAZ CONSOLE?" << std::endl;

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
	while (true) {

	}

	return EXIT_SUCCESS;
}
