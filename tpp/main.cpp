#include <iostream>

#include "vterm/virtual_terminal.h"

#include "settings.h"

// include all renderers, the renderers themselves contain the guards that selects only one
#include "gdi/gdi_application.h"
#include "gdi/gdi_terminal_window.h"

#include "conpty/conpty_connector.h"

#include "helpers/object.h"


#include "helpers/hash.h"

namespace tpp {

	SettingsSingleton Settings;

} // namespace tpp

using namespace tpp;

Application * application;

//HINSTANCE hInstance;
//TerminalWindow * tw;

// https://docs.microsoft.com/en-us/windows/desktop/gdi/windows-gdi


// https://docs.microsoft.com/en-us/windows/desktop/api/_gdi/

/** Terminal++ App Entry Point

    For now creates single terminal window and one virtual terminal. 
 */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	helpers::HashMD5 md5("56ab781256ab781256ab781256ab7812");
	std::cout << md5 << std::endl;

	std::unordered_map<helpers::HashMD5, std::string> x;


	// create the application
	application = new GDIApplication(hInstance);

	// create the screen buffer
	vterm::VirtualTerminal * vterm1 = new vterm::VirtualTerminal();



	// and create the terminal window
	TerminalWindow * tw = application->createNewTerminalWindow();
	tw->setTerminal(vterm1);
	tw->show();

	ConPTYConnector c("wsl -e echo hello", vterm1);
	//ConPTYConnector c("wsl -e ls /home/peta/devel -la", vterm1);
	//ConPTYConnector c("wsl -e ping www.seznam.cz", vterm1);
	//ConPTYConnector c("wsl -e mc", vterm1);
	//ConPTYConnector c("wsl -e screenfetch", vterm1);
	// Now run the main loop
	application->mainLoop();

	return EXIT_SUCCESS;
}
