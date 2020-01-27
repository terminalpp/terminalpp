﻿#include <iostream>

#include "helpers/log.h"
#include "helpers/char.h"
#include "helpers/time.h"
#include "helpers/filesystem.h"

#include "config.h"

#if (defined ARCH_WINDOWS)

#include "directwrite/directwrite_application.h"
#include "directwrite/directwrite_window.h"
// link to directwrite
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

#elif (defined ARCH_UNIX)


#include "x11/x11_application.h"
#include "x11/x11_window.h"

#else
#error "Unsupported platform"
#endif

#include "forms/session.h"

#include "ui/widgets/panel.h"
#include "ui/widgets/label.h"
#include "ui/builders.h"
#include "ui/layout.h"

#include "ui-terminal/bypass_pty.h"
#include "ui-terminal/local_pty.h"
#include "ui-terminal/terminal.h"
#include <thread>


#include "helpers/json.h"

#include "helpers/filesystem.h"

using namespace tpp;

void reportError(std::string const & message) {
#if (defined ARCH_WINDOWS)
    helpers::utf16_string text = helpers::UTF8toUTF16(message);
	MessageBox(nullptr, text.c_str(), L"Fatal Error", MB_ICONSTOP);
#else
	std::cout << message << std::endl;
#endif
}


// https://www.codeguru.com/cpp/misc/misc/graphics/article.php/c16139/Introduction-to-DirectWrite.htm

// https://docs.microsoft.com/en-us/windows/desktop/gdi/windows-gdi

// https://docs.microsoft.com/en-us/windows/desktop/api/_gdi/

// https://github.com/Microsoft/node-pty/blob/master/src/win/conpty.cc

/** Terminal++ App Entry Point

    For now creates single terminal window and one virtual terminal. 
 */
#ifdef ARCH_WINDOWS
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	MARK_AS_UNUSED(hPrevInstance);
	MARK_AS_UNUSED(lpCmdLine);
	MARK_AS_UNUSED(nCmdShow);
	int argc = __argc;
	char** argv = __argv;
	DirectWriteApplication::Initialize(hInstance);
#else
int main(int argc, char* argv[]) {
    try {
	    X11Application::Initialize();
    } catch (helpers::Exception const & e) {
        std::cerr << e << std::endl;
        return EXIT_FAILURE;
    }
#endif
	try {
		Config const & config = Config::Setup(argc, argv);
		// make sure the log & remote files directories exist
		helpers::CreatePath(config.log.dir());
		helpers::CreatePath(config.session.remoteFiles.dir());
		// check that the logs directory does not overgrow the max number of files allowed
		helpers::EraseOldestFiles(config.log.dir(), config.log.maxFiles());
		// create log writer & enable the selected logs
		helpers::Logger::FileWriter log(helpers::UniqueNameIn(config.log.dir(), "log-"));
		helpers::Logger::Enable(log, { 
			helpers::Logger::DefaultLog(),
			ui::Terminal::SEQ_ERROR,
			ui::Terminal::SEQ_UNKNOWN
		});
		LOG() << "t++ started";
		// Create the palette & the pty - TODO this should be more systematic
		ui::Terminal::Palette palette{config.session.palette()};
		ui::Terminal::PTY * pty;
#if (defined ARCH_WINDOWS)
		if (config.session.pty() != "bypass") 
		    pty = new LocalPTY(config.session.command());
		else
		    pty = new BypassPTY(config.session.command());
#else
		pty = new LocalPTY(config.session.command());
#endif

		// create the session
		Session * session = new Session(pty, & palette);

		// and create the main window

	    tpp::Window * w = Application::Instance()->createWindow(DEFAULT_WINDOW_TITLE, config.session.cols(), config.session.rows(), config.font.size());
		w->setRootWindow(session);
		w->show();
		if (config.session.fullscreen())
		    w->setFullscreen(true);
    	Application::Instance()->mainLoop();
		delete session;

	    return EXIT_SUCCESS;
	} catch (helpers::Exception const& e) {
		Application::Alert(STR(e));
		LOG() << "Error: " << e;
	} catch (std::exception const& e) {
		Application::Alert(e.what());
		LOG() << "Error: " << e.what();
	} catch (...) {
		Application::Alert("Unknown error");
		LOG() << "Error: Unknown error";
	} 
	return EXIT_FAILURE;
}