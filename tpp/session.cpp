#include "vterm/local_pty.h"
#include "vterm/vt100.h"

#include "application.h"

#include "session.h"


namespace tpp {

	Session::Session(std::string const & name, helpers::Command const & command) :
		name_(name),
		command_(command),
		pty_(nullptr),
		windowProperties_(Application::Instance()->defaultTerminalWindowProperties()) {
	}


	void Session::start() {
		ASSERT(pty_ == nullptr) << "Session " << name_ << " already started";
		// create the VT100 decoder, and associated PTY backend
		pty_ = new vterm::LocalPTY(command_);
		vterm::VT100 * vt = new vterm::VT100(
			pty_,
			vterm::Palette::ColorsXTerm256(), 
			15,
			0);
		// create the terminal
		terminal_ = new vterm::Terminal(windowProperties_.cols, windowProperties_.rows, vt);
		// start the thread feeding the results

		// TODO this is wrong and should actually use messages in Application
		std::thread tt([vt]() {
			while (vt->waitForInput()) {
				vt->processInput();
			}
			});
		tt.detach();

		// and create the terminal window
		window_ = Application::Instance()->createTerminalWindow(windowProperties_, name_);
		window_->setTerminal(terminal_);
	}

} // namespace tpp