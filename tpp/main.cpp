#include <iostream>

#include "helpers/log.h"


#ifdef WIN32
#include "win32/pty_terminal.h"
#include "win32/application.h"
#include "win32/terminal_window.h"
// link to directwrite
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#elif __linux__
#include "linux/pty_terminal.h"
#include "linux/application.h"
#include "linux/terminal_window.h"
#else
#error "Unsupported platform"
#endif



using namespace tpp;

// https://www.codeguru.com/cpp/misc/misc/graphics/article.php/c16139/Introduction-to-DirectWrite.htm

// https://docs.microsoft.com/en-us/windows/desktop/gdi/windows-gdi

// https://docs.microsoft.com/en-us/windows/desktop/api/_gdi/

// https://github.com/Microsoft/node-pty/blob/master/src/win/conpty.cc


void FixMissingSettings(TerminalSettings & ts) {
	vterm::Font defaultFont;
	TerminalWindow::Font* f = TerminalWindow::Font::GetOrCreate(defaultFont, ts.defaultFontHeight, 1);
	ts.defaultFontWidth = f->widthPx();
}



/** Terminal++ App Entry Point

    For now creates single terminal window and one virtual terminal. 
 */
#ifdef WIN32

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {

	Application* app = new Application(hInstance);

	TerminalSettings ts;
	ts.defaultCols = 80;
	ts.defaultRows = 25;
	ts.defaultFontHeight = 18;
	ts.defaultFontWidth = 0;
	ts.defaultZoom = 1;

	FixMissingSettings(ts);

	std::cout << "OH HAI, CAN I HAZ CONSOLE?" << std::endl;


	// initialize log levels we use
	//helpers::Log::RegisterLogger((new helpers::StreamLogger(vterm::VT100::SEQ, std::cout))->toFile("c:/delete/tpp.txt"));
	helpers::Log::RegisterLogger((new helpers::StreamLogger(vterm::VT100::UNKNOWN_SEQ, std::cout)));

	TerminalWindow * tw = new TerminalWindow(app, &ts);
	tw->show();


	//Terminal * t = new Terminal("wsl -e echo hello mmoo", 80, 25);
	//Terminal * t = new Terminal("wsl -e ping www.seznam.cz", 80, 25);
	//Terminal * t = new Terminal("wsl -e screenfetch", 80, 25, vterm::Palette::Colors16, 15, 0);
	//Terminal * t = new Terminal("wsl -e bash", 80, 25, vterm::Palette::Colors16, 15, 0);
	//Terminal* t = new Terminal("wsl -e infocmp", 80, 25, vterm::Palette::Colors16, 15, 0);
	Terminal* t = new Terminal("wsl -e mc", 80, 25, vterm::Palette::Colors16, 15, 0);
    //Terminal * t = new Terminal("wsl -e emacs ~/settings/emacs/init.el", 80, 25, vterm::Palette::Colors16, 15, 0);
	//Terminal * t = new Terminal("wsl -e bash -c \"ssh orange \"", 80, 25, vterm::Palette::Colors16, 15, 0);

	t->execute();

	tw->attachTerminal(t);

	app->mainLoop();

	return EXIT_SUCCESS;
}

#elif __linux__

int main(int argc, char* argv[]) {
	try {

		Application* app = new Application();

		TerminalSettings ts;
		ts.defaultCols = 100;
		ts.defaultRows = 40;
		ts.defaultFontHeight = 18;
		ts.defaultFontWidth = 0;
		ts.defaultZoom = 1;

		helpers::Log::RegisterLogger((new helpers::StreamLogger(vterm::VT100::SEQ, std::cout)));
		helpers::Log::RegisterLogger((new helpers::StreamLogger(vterm::VT100::UNKNOWN_SEQ, std::cout)));


		FixMissingSettings(ts);


		TerminalWindow* tw = new TerminalWindow(app, &ts);
		tw->show();

		//Terminal* t = new Terminal("./check-tty.sh", { }, 100, 40, vterm::Palette::Colors16, 15, 0);
		//Terminal* t = new Terminal("printenv", {  }, 100, 40, vterm::Palette::Colors16, 15, 0);
		//Terminal* t = new Terminal("tput", { "colors" }, 100, 40, vterm::Palette::Colors16, 15, 0);
		//Terminal* t = new Terminal("ls", { "-la" }, 100, 40, vterm::Palette::Colors16, 15, 0);
		//Terminal* t = new Terminal("infocmp", {  }, 100, 40, vterm::Palette::Colors16, 15, 0);
		Terminal* t = new Terminal("screenfetch", {  }, 100, 40, vterm::Palette::Colors16, 15, 0);
		//Terminal* t = new Terminal("mc", { }, 100, 40, vterm::Palette::Colors16, 15, 0);

		tw->attachTerminal(t);

		t->execute();


		app->mainLoop();
		return EXIT_SUCCESS;
	} catch (helpers::Exception const& e) {
		std::cout << e;
	}
	catch (std::exception const& e) {
		std::cout << e.what();
	} 
	catch (...) {
		std::cout << "unknown error";
	}
	return EXIT_FAILURE;
} 

#endif
