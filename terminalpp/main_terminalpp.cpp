﻿#include <iostream>
#include <thread>



#include "helpers/char.h"
#include "helpers/time.h"
#include "helpers/filesystem.h"
#include "helpers/curl.h"

#include "config.h"

#if (defined ARCH_WINDOWS && defined RENDERER_NATIVE)

#include "directwrite/directwrite_application.h"
#include "directwrite/directwrite_window.h"
// link to directwrite
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

#define APPLICATION_CLASS DirectWriteApplication

#elif (defined ARCH_UNIX && defined RENDERER_NATIVE)

#include "x11/x11_application.h"
#include "x11/x11_window.h"

#define APPLICATION_CLASS X11Application

#elif (defined RENDERER_QT)

#include "qt/qt_application.h"
#include "qt/qt_window.h"

#define APPLICATION_CLASS QtApplication

#else
#error "Unsupported platform"
#endif

#include "forms/session.h"


#include "helpers/json.h"

#include "helpers/filesystem.h"

void reportError(std::string const & message) {
#if (defined ARCH_WINDOWS)
    utf16_string text = UTF8toUTF16(message);
	MessageBox(nullptr, text.c_str(), L"Fatal Error", MB_ICONSTOP);
#else
	std::cout << message << std::endl;
#endif
}

void PrintVersion() {
    std::cout << "Terminal++ version " << stamp::version << std::endl;
    std::cout << "    commit:   " << stamp::commit << (stamp::dirty ? "*" : "") << std::endl;
    std::cout << "              " << stamp::build_time << std::endl;
#if (defined RENDERER_QT)
    std::cout << "    platform: " << ARCH << "(Qt) " << ARCH_SIZE << " " << ARCH_COMPILER << " " << ARCH_COMPILER_VERSION << " " << stamp::build << std::endl;
#else
    std::cout << "    platform: " << ARCH << "(native) " << ARCH_SIZE << " " << ARCH_COMPILER << " " << ARCH_COMPILER_VERSION << " " << stamp::build << std::endl;
#endif
    exit(EXIT_SUCCESS);
}

// https://www.codeguru.com/cpp/misc/misc/graphics/article.php/c16139/Introduction-to-DirectWrite.htm

// https://docs.microsoft.com/en-us/windows/desktop/gdi/windows-gdi

// https://docs.microsoft.com/en-us/windows/desktop/api/_gdi/

// https://github.com/Microsoft/node-pty/blob/master/src/win/conpty.cc

/** Terminal++ App Entry Point

    For now creates single terminal window and one virtual terminal. 
 */
#if (defined ARCH_WINDOWS && defined RENDERER_NATIVE)
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	MARK_AS_UNUSED(hPrevInstance);
	MARK_AS_UNUSED(lpCmdLine);
	MARK_AS_UNUSED(nCmdShow);
	int argc = __argc;
	char** argv = __argv;
    CheckVersion(argc, argv, PrintVersion);
	tpp::APPLICATION_CLASS::Initialize(argc, argv, hInstance);
#elif (defined ARCH_WINDOWS && defined RENDERER_QT)
int main(int argc, char* argv[]) {
    CheckVersion(argc, argv, PrintVersion);
	tpp::APPLICATION_CLASS::Initialize(argc, argv);
#else
int main(int argc, char* argv[]) {
    CheckVersion(argc, argv, PrintVersion);
	tpp::APPLICATION_CLASS::Initialize(argc, argv);
#endif
    try {
        tpp::Config::Setup(argc, argv);
        //JSON versions{tpp::Application::Instance()->checkLatestVersion("edge")};        

		//Logger::FileWriter log(UniqueNameIn(config.log.dir(), "log-"));
		Logger::Enable(Logger::StdOutWriter(), { 
			Log::Default(),
			ui::AnsiTerminal::SEQ_ERROR,
			ui::AnsiTerminal::SEQ_UNKNOWN
		});
		LOG() << "t++ started";

        tpp::Window * w = tpp::Application::Instance()->createWindow("Foobar", 80, 25);
        // currently owned by the window, when multiple sessions are available this might change
        new tpp::Session{w};
        w->show();
        tpp::Application::Instance()->mainLoop();
        return EXIT_SUCCESS;
	} catch (Exception const& e) {
		tpp::Application::Instance()->alert(STR(e));
		LOG() << "Error: " << e;
	} catch (std::exception const& e) {
		tpp::Application::Instance()->alert(e.what());
		LOG() << "Error: " << e.what();
	} catch (...) {
		tpp::Application::Instance()->alert("Unknown error");
		LOG() << "Error: Unknown error";
	} 
	return EXIT_FAILURE;
    /*

	Config const & config = Config::Setup(argc, argv);

	try {
		Config const & config = Config::Setup(argc, argv);
		// make sure the log & remote files directories exist
		CreatePath(config.log.dir());
		CreatePath(config.session.remoteFiles.dir());
		// check that the logs directory does not overgrow the max number of files allowed
		EraseOldestFiles(config.log.dir(), config.log.maxFiles());
		// create log writer & enable the selected logs
		Logger::FileWriter log(UniqueNameIn(config.log.dir(), "log-"));
		Logger::Enable(log, { 
			Logger::DefaultLog(),
			ui::Terminal::SEQ_ERROR,
			ui::Terminal::SEQ_UNKNOWN,
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
	} catch (Exception const& e) {
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
    */ 
}