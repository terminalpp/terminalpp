#include <iostream>

#include "vterm/screen_buffer.h"
#include "vterm/virtual_terminal.h"

#include "settings.h"

// include all renderers, the renderers themselves contain the guards that selects only one
#include "gdi/application.h"
#include "gdi/terminal_window.h"

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

/** Terminal++ App Entry Point

    For now creates single terminal window and one virtual terminal. 
 */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {



	// create the application
	application = new Application(hInstance);

	// create the screen buffer
	vterm::ScreenBuffer * vterm1 = new vterm::ScreenBuffer();

	// and create the terminal window
	TerminalWindow * tw = application->createNewTerminalWindow(vterm1);
	tw->show();

	// Now run the main loop
	application->mainLoop();

	return EXIT_SUCCESS;
}
