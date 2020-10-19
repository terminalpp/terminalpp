#include <iostream>
#include <thread>

#include "helpers/char.h"
#include "helpers/time.h"
#include "helpers/filesystem.h"
#include "helpers/curl.h"
#include "helpers/telemetry.h"

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

#include "forms/terminal_window.h"


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
#if (defined ARCH_WINDOWS && defined RENDERER_NATIVE)
    // Make sure there is either a terminal, to which the stdout goes
    tpp::AttachConsole();
#endif
    std::cout << tpp::Application::Stamp();
    exit(EXIT_SUCCESS);
}

/** Determines whether there is telemetry information to be sent and raised as a bugfix. 

    For now, this can only be done in case of fatal errors. 
 */
void SendTelemetry(Telemetry & telemetry) {
    if (telemetry.messages(FATAL_ERROR) > 0) {
        if (tpp::Application::Instance()->query("Send telemetry?", "Do you want to copy the location of telemetry log to clipboard and fill in an issue?")) {
            telemetry.setKeepAfterClosing();
            tpp::Application::Instance()->setClipboard(telemetry.filename());
            tpp::Application::Instance()->createNewIssue("", STR("Please check that a similar bug has not been already filed. If not, fill in the description and title of the bug, keeping the version information below. If possible, please attach the telemetry log (in file " << telemetry.filename() << "), whose file location has been copied to your clipboard. Thank you!"));
        }
    }
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
    // create the telemetry manager and its handler. 
    Telemetry telemetry(SendTelemetry);
    try {
        //tpp::Config const & config = tpp::Config::Setup(argc, argv);
        tpp::Config const & config = tpp::Config::Setup(argc, argv);
        // open the telemetry and add the registered logs
        telemetry.open(config.telemetry.dir() + "/" + TimeInDashed());
        for (auto & i : config.telemetry.events())
            telemetry.addLog(i);

		Log::Enable(Log::StdOutWriter(), { 
			Log::Default(),
            Log::Exception(),
			ui::AnsiTerminal::SEQ_ERROR,
			ui::AnsiTerminal::SEQ_UNKNOWN
		});

        tpp::Window * w = tpp::Application::Instance()->createWindow("Foobar", config.renderer.window.cols(), config.renderer.window.rows());
        if (config.renderer.window.fullscreen())
            w->setFullscreen(true);
        // currently owned by the window, when multiple sessions are available this might change
        tpp::TerminalWindow * tw = new tpp::TerminalWindow{w};
        tw->newSession(config.sessionByName(config.defaultSession()));
        //tw->newSession(config.sessions[0]);
        //new tpp::Session{w, config.sessionByName(config.defaultSession())};
        w->show();
#if (defined ARCH_WINDOWS && defined RENDERER_NATIVE)
        // TODO se how fast this is and perhaps execute in separate thread?
        if (config.application.checkProfileShortcuts()) {
            w->schedule([](){
                tpp::DirectWriteApplication::Instance()->updateProfilesJumplist();
            });
        }
#endif
        tpp::Application::Instance()->mainLoop();
        return EXIT_SUCCESS;
	} catch (Exception const& e) {
        LOG(FATAL_ERROR) << e;
		tpp::Application::Instance()->alert(STR(e));
	} catch (std::exception const& e) {
        LOG(FATAL_ERROR) << e.what();
		tpp::Application::Instance()->alert(e.what());
		LOG() << "Error: " << e.what();
	} catch (...) {
        LOG(FATAL_ERROR) << "unknown exception raised";
		tpp::Application::Instance()->alert("Unknown error");
	} 
	return EXIT_FAILURE;
}
