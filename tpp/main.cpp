#include <iostream>

#include "vterm/vt100.h"
#include "vterm/win32/conpty_terminal.h"

#include "application.h"
#include "terminal_window.h"


using namespace tpp;

// https://www.codeguru.com/cpp/misc/misc/graphics/article.php/c16139/Introduction-to-DirectWrite.htm

// https://docs.microsoft.com/en-us/windows/desktop/gdi/windows-gdi

// https://docs.microsoft.com/en-us/windows/desktop/api/_gdi/

class Term : public vterm::VT100, public vterm::ConPTYTerminal {
public:
	Term(std::string const & cmd, unsigned cols, unsigned rows) :
		VT100(cols, rows),
		ConPTYTerminal(cmd, cols, rows),
		PTYTerminal(cols, rows) {
	}

};

/** Terminal++ App Entry Point

    For now creates single terminal window and one virtual terminal. 
 */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {


	Application app(hInstance);
	std::cout << "OH HAI, CAN I HAZ CONSOLE?" << std::endl;


	//vterm::ConPTYTerminal * t(new vterm::ConPTYTerminal("wsl -e echo hello", 80, 25));
	Term * t = new Term("wsl -e echo hello", 80, 25);
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
	app.mainLoop();

	return EXIT_SUCCESS;
}
