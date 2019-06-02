#include <iostream>

#include "helpers/log.h"

#include "vterm/local_pty.h"
#include "vterm/vt100.h"

#include "session.h"


#ifdef WIN32
#include "gdi/gdi_application.h"
#include "gdi/gdi_terminal_window.h"
// link to directwrite
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#elif __linux__
#include "x11/x11_application.h"
#include "x11/x11_terminal_window.h"
#else
#error "Unsupported platform"
#endif



using namespace tpp;

// https://www.codeguru.com/cpp/misc/misc/graphics/article.php/c16139/Introduction-to-DirectWrite.htm

// https://docs.microsoft.com/en-us/windows/desktop/gdi/windows-gdi

// https://docs.microsoft.com/en-us/windows/desktop/api/_gdi/

// https://github.com/Microsoft/node-pty/blob/master/src/win/conpty.cc


/*
void FixMissingSettings(TerminalSettings& ts) {
	vterm::Font defaultFont;
	X11TerminalWindow::Font* f = X11TerminalWindow::Font::GetOrCreate(defaultFont, ts.defaultFontHeight, 1);
	ts.defaultFontWidth = f->widthPx();
}
*/


/** Terminal++ App Entry Point

    For now creates single terminal window and one virtual terminal. 
 */
#ifdef WIN32

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	// create the application singleton
	new GDIApplication(hInstance);

	Session* s = new Session("wsl", helpers::Command("wsl", { "-e", "bash" }));
	s->start();
	s->show();

	Application::MainLoop();

	return EXIT_SUCCESS;

}

#elif __linux__

int main(int argc, char* argv[]) {
	try {

		// create the application singleton
		new X11Application();

		Session* s = new Session("bash", helpers::Command("bash", {}));
		s->start();
		s->show();

		Application::MainLoop();

		return EXIT_SUCCESS;


		/*



		X11Application* app = new X11Application();

		TerminalSettings ts;
		ts.defaultCols = 100;
		ts.defaultRows = 40;
		ts.defaultFontHeight = 18;
		ts.defaultFontWidth = 0;
		ts.defaultZoom = 1;

		//helpers::Log::RegisterLogger((new helpers::StreamLogger(vterm::VT100::SEQ, std::cout)));
		helpers::Log::RegisterLogger((new helpers::StreamLogger(vterm::VT100::SEQ_UNKNOWN, std::cout)));


		FixMissingSettings(ts);


		//TerminalWindow* tw = new TerminalWindow(app, &ts);
		//tw->show();

		//Terminal* t = new Terminal("./check-tty.sh", { }, 100, 40, vterm::Palette::Colors16, 15, 0);
		//Terminal* t = new Terminal("printenv", {  }, 100, 40, vterm::Palette::Colors16, 15, 0);
		//Terminal* t = new Terminal("tput", { "colors" }, 100, 40, vterm::Palette::Colors16, 15, 0);
		//Terminal* t = new Terminal("ls", { "-la" }, 100, 40, vterm::Palette::Colors16, 15, 0);
		//Terminal* t = new Terminal("infocmp", {  }, 100, 40, vterm::Palette::Colors16, 15, 0);
		//Terminal* t = new Terminal("screenfetch", {  }, 100, 40, vterm::Palette::Colors16, 15, 0);
		//Terminal* t = new Terminal("mc", { }, 100, 40, vterm::Palette::Colors16, 15, 0);
		//Terminal* t = new Terminal("bash", { }, 100, 40, vterm::Palette::ColorsXTerm256(), 15, 0);

		//tw->attachTerminal(t);

		//t->execute();

        vterm::Terminal * t = new vterm::Terminal(80, 25);

        X11TerminalWindow * tw = new X11TerminalWindow(app, &ts);
        tw->show();

        tw->setTerminal(t);




        vterm::VT100* vt100 = new vterm::VT100(
			new vterm::LocalPTY(helpers::Command("bash", {})),
			vterm::Palette::ColorsXTerm256(), 15, 0
		);
		t->setBackend(vt100);
        //vt100->setTerminal(t);
        //vt100->startThreadedReceiver();
		std::thread tt([&]() {
			while (vt100->waitForInput()) {
				vt100->processInput();
			}
			LOG << "terminated";
			});
		tt.detach();


		app->mainLoop();
		return EXIT_SUCCESS;

		*/
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
