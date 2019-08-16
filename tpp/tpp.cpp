#include <iostream>

#include "helpers/log.h"
#include "helpers/char.h"
#include "helpers/args.h"
#include "helpers/time.h"

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

#include "ui/widgets/panel.h"
#include "ui/widgets/label.h"
#include "ui/widgets/scrollbox.h"
#include "ui/builders.h"
#include "ui/layout.h"

#include "vterm/terminal.h"
#include "vterm/bypass_pty.h"
#include "vterm/local_pty.h"
#include "vterm/vt100.h"
#include <thread>


#include "stamp.h"
#include "helpers/stamp.h"



namespace tpp {
	namespace config {
#ifdef ARCH_WINDOWS
		helpers::Arg<bool> UseConPTY(
			{ "--use-conpty" },
			false,
			false,
			"Uses the Win32 ConPTY pseudoterminal instead of the WSL bypass"
		);
#endif
		helpers::Arg<unsigned> FPS(
			{ "--fps" },
			DEFAULT_TERMINAL_FPS,
			false,
			"Maximum number of fps the terminal will display"
		);
		helpers::Arg<unsigned> Cols(
			{ "--cols", "-c" },
			DEFAULT_TERMINAL_COLS,
			false,
			"Number of columns of the terminal window"
		);
		helpers::Arg<unsigned> Rows(
			{ "--rows", "-r" },
			DEFAULT_TERMINAL_ROWS,
			false,
			"Number of rows of the terminal window"
		);
		helpers::Arg<std::string> FontFamily(
			{ "--font" }, 
			DEFAULT_TERMINAL_FONT_FAMILY, 
			false, 
			"Font to render the terminal with"
		);
		helpers::Arg<unsigned> FontSize(
			{ "--font-size" }, 
			DEFAULT_TERMINAL_FONT_SIZE,
			false,
			"Size of the font in pixels at no zoom."
		);
		helpers::Arg<std::vector<std::string>> Command(
			{ "-e" }, 
			{ DEFAULT_TERMINAL_COMMAND },
			false,
			"Determines the command to be executed in the terminal",
			true
		);
		helpers::Arg<std::string> RecordSession(
			{ "--record" },
			"",
			false,
			"File to which the session input (output of the attached process) should be recorded"
		);
	}
}


using namespace tpp;

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
#else
int main(int argc, char* argv[]) {
#endif
	helpers::Arguments::SetVersion(STR("t++ :" << helpers::Stamp::Stored()));
	helpers::Arguments::Parse(argc, argv);
	try {
	    // create the application singleton
#ifdef ARCH_WINDOWS
	    DirectWriteApplication::Initialize(hInstance);
#else
	    X11Application::Initialize();
#endif
		//helpers::Log::RegisterLogger(new helpers::StreamLogger(vterm::VT100::SEQ, std::cout));
		//helpers::Log::RegisterLogger(new helpers::StreamLogger(vterm::VT100::SEQ_UNKNOWN, std::cout));
		//helpers::Log::RegisterLogger(new helpers::StreamLogger(vterm::VT100::SEQ_WONT_SUPPORT, std::cout));

		vterm::VT100::Palette palette(vterm::VT100::Palette::XTerm256());

		vterm::VT100 * vt100;

		vterm::PTY * pty;
#if (defined ARCH_WINDOWS)
		if (*config::UseConPTY) { 
			if (!config::Command.specified())
			    (*config::Command) = std::vector<std::string>{DEFAULT_CONPTY_COMMAND};
		    pty = new vterm::LocalPTY(helpers::Command(*config::Command));
		} else {
		    pty = new vterm::BypassPTY(helpers::Command(*config::Command));
		}
#else
		pty = new vterm::LocalPTY(helpers::Command(*config::Command));
#endif

	    tpp::Window * w = Application::Instance()->createWindow(DEFAULT_WINDOW_TITLE, * config::Cols, * config::Rows, *config::FontSize);
		ui::RootWindow* rw = ui::Create<ui::RootWindow>()
			<< ui::Layout::Horizontal()
			<< (ui::Create<ui::Label>()
				<< ui::HeightHint(ui::SizeHint::Fixed())
				<< ui::Geometry(1, 1)
				<< STR("t++ :" << helpers::Stamp::Stored())
				<< ui::Visibility(false)
				)
			<< (
				ui::Create(vt100 = new vterm::VT100(*config::Cols, *config::Rows, &palette, pty))
				);

		w->setRootWindow(rw);
		w->show();
		vt100->setFocus(true);

    	Application::Instance()->mainLoop();

		delete rw;

	    return EXIT_SUCCESS;
	} catch (helpers::Exception const& e) {
		std::cout << e;
	} /*catch (std::exception const& e) {
		std::cout << e.what();
	} catch (...) {
		std::cout << "unknown error";
	} */
	return EXIT_FAILURE;
}
