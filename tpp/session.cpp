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
		terminal_(nullptr),
		windowProperties_(Application::Instance<>()->defaultTerminalWindowProperties()) {
		Sessions_.insert(this);
	}

	Session::~Session() {
		if (pty_ != nullptr) { // when ptty is null the sesion did not even start and everyone else will be null as well
			// terminate the pty
			pty_->terminate();
			// detach the window from the terminal
			window_->setTerminal(nullptr);
            LOG << "Window terminal set to null";
			// deleting the terminal deletes the terminal, backend and pty
			delete terminal_;
		}
	}

	void Session::start() {
		ASSERT(pty_ == nullptr) << "Session " << name_ << " already started";

		// create the terminal window
		window_ = Application::Instance()->createTerminalWindow(this, windowProperties_, name_);
		// create the PTY and the terminal
#ifdef ARCH_WINDOWS
		if (*config::UseConPTY)
			pty_ = new vterm::LocalPTY(command_);
		else
			pty_ = new vterm::BypassPTY(command_);
#else 
		pty_ = new vterm::LocalPTY(command_);
#endif
		pty_->onTerminated += HANDLER(Session::onPTYTerminated);
		if (config::RecordSession.specified()) {
			pty_->recordInput(*config::RecordSession);
			LOG << "Session input recorded to " << *config::RecordSession;
		}
		// create the terminal backend
		terminal_ = new vterm::VT100(window_->cols(), window_->rows(), pty_);
		/*
			pty_,
			vterm::Palette::ColorsXTerm256(), 
			15,
			0);

			*/
		// create the terminal
		//terminal_ = new vterm::Terminal(windowProperties_.cols, windowProperties_.rows, vt_);
		window_->setTerminal(terminal_);
	}

} // namespace tpp