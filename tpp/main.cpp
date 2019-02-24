#include <iostream>

#include "vterm/virtual_terminal.h"

#include "settings.h"

// include all renderers, the renderers themselves contain the guards that selects only one
#include "gdi/gdi_application.h"
#include "gdi/gdi_terminal_window.h"

#include "helpers/object.h"

namespace tpp {

	SettingsSingleton Settings;

} // namespace tpp

using namespace tpp;

Application * application;


//HINSTANCE hInstance;
//TerminalWindow * tw;

// https://docs.microsoft.com/en-us/windows/desktop/gdi/windows-gdi


// https://docs.microsoft.com/en-us/windows/desktop/api/_gdi/

void haha(vterm::RepaintEvent & e) {

}

/** Terminal++ App Entry Point

    For now creates single terminal window and one virtual terminal. 
 */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {

	// create the application
	application = new GDIApplication(hInstance);

	// create the screen buffer
	Terminal * vterm1 = new Terminal();
	vterm1->onRepaint += HANDLER(haha);

	// and create the terminal window
	TerminalWindow * tw = application->createNewTerminalWindow();
	tw->setTerminal(vterm1);
	tw->show();

	// Now run the main loop
	application->mainLoop();

	return EXIT_SUCCESS;
}
