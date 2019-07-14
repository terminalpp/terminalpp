#include <iostream>

#include "helpers/log.h"

#include "vterm/local_pty.h"
#include "vterm/vt100.h"

#include "config.h"
#include "session.h"

#include "helpers/char.h"
#include "helpers/args.h"

#ifdef _WIN64

#include "directwrite/directwrite_application.h"
#include "directwrite/directwrite_terminal_window.h"
// link to directwrite
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

#elif (defined __linux__) || (defined __APPLE__)

#include "x11/x11_application.h"
#include "x11/x11_terminal_window.h"

#else
#error "Unsupported platform"
#endif

#include "helpers/time.h"

using namespace tpp;

// https://www.codeguru.com/cpp/misc/misc/graphics/article.php/c16139/Introduction-to-DirectWrite.htm

// https://docs.microsoft.com/en-us/windows/desktop/gdi/windows-gdi

// https://docs.microsoft.com/en-us/windows/desktop/api/_gdi/

// https://github.com/Microsoft/node-pty/blob/master/src/win/conpty.cc

// TODO add real arguments
helpers::Arg<std::string> SessionPTY({ "-pty" }, "local", false, "Determines which pty to use");
helpers::Arg<std::vector<std::string>> SessionCommand({ "-e" }, { "bash" }, false, "Determines the command to be executed in the terminal", true);

/** Terminal++ App Entry Point

    For now creates single terminal window and one virtual terminal. 
 */
#ifdef _WIN64
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	MARK_AS_UNUSED(hPrevInstance);
	MARK_AS_UNUSED(lpCmdLine);
	MARK_AS_UNUSED(nCmdShow);
	try {
		helpers::Arguments::Parse(__argc, __argv);
	    // create the application singleton
	    new DirectWriteApplication(hInstance);
#elif (defined __linux__) || (defined __APPLE__)
int main(int argc, char* argv[]) {
	try {
		helpers::Arguments::Parse(argc, argv);
		// create the application singleton
	    new X11Application();
#endif

		//helpers::Log::RegisterLogger(new helpers::StreamLogger(vterm::VT100::SEQ, std::cout));
		helpers::Log::RegisterLogger(new helpers::StreamLogger(vterm::VT100::SEQ_UNKNOWN, std::cout));
		helpers::Log::RegisterLogger(new helpers::StreamLogger(vterm::VT100::SEQ_WONT_SUPPORT, std::cout));

		Session* s = Session::Create("t++", DEFAULT_SESSION_COMMAND);
		s->start();
		s->show();

    	Application::MainLoop();

	    return EXIT_SUCCESS;
	} catch (helpers::Exception const& e) {
		std::cout << e;
	} catch (std::exception const& e) {
		std::cout << e.what();
	} catch (...) {
		std::cout << "unknown error";
	}
	return EXIT_FAILURE;
}
