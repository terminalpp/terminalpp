#include "vterm/local_pty.h"
#include "vterm/bypass_pty.h"
#include "vterm/vt100.h"

#include "application.h"

#include "session.h"


namespace tpp {

	std::unordered_set<Session*> Session::Sessions_;

	Session::Session(std::string const& name, helpers::Command const& command) :
		closing_(false),
		name_(name),
		command_(command),
		pty_(nullptr),
		windowProperties_(Application::Instance<>()->defaultTerminalWindowProperties()),
	    dataReady_(false) {
		Sessions_.insert(this);
	}

	Session::~Session() {
		if (pty_ != nullptr) { // when ptty is null the sesion did not even start and everyone else will be null as well
			// terminate the pty
			pty_->terminate();
			ptyExitWait_.join();
			// there can be unprocessed data left, which we happily ignore so we wake up the read thread with closing_ set to true, which should exit it immediately
			cvPty_.notify_all();
			ptyReadThread_.join();
			// detach the window from the terminal
			window_->setTerminal(nullptr);
            LOG << "Window terminal set to null";
			// deleting the terminal deletes the terminal, backend and pty
			delete terminal_;
		}
	}

	void Session::start() {
		ASSERT(pty_ == nullptr) << "Session " << name_ << " already started";
		// create the VT100 decoder, and associated PTY backend
		pty_ = new DEFAULT_SESSION_PTY(command_);
		// create the thread waiting for the PTY to terminate
        // TODO no need for extra thread - attach the handler to PTY
		ptyExitWait_ = std::thread([this]() {
			helpers::ExitCode ec = pty_->waitFor();
			onPTYTerminated(ec);
			LOG << "process exit monitor finished";
		});
		// create the terminal backend
		vt_ = new DEFAULT_SESSION_BACKEND(
			pty_,
			vterm::Palette::ColorsXTerm256(), 
			15,
			0);
		// create the terminal
		terminal_ = new vterm::Terminal(windowProperties_.cols, windowProperties_.rows, vt_);
		// create the terminal window
		window_ = Application::Instance()->createTerminalWindow(this, windowProperties_, name_);
		window_->setTerminal(terminal_);
		// start the thread feeding the results
		ptyReadThread_ = std::thread([this]() {
			std::unique_lock<std::mutex> l(mPty_);
			while (vt_->waitForInput()) {
				dataReady_ = true;
				window_->inputReady();
				while (dataReady_) {
					cvPty_.wait(l);
					if (closing_)
						return;
				}
			}
		});
	}

} // namespace tpp