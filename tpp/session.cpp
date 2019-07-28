#include "vterm/local_pty.h"
#include "vterm/bypass_pty.h"
#include "vterm/vt100.h"

#include "application.h"

#include "session.h"

#include "ui/root_window.h"
#include "ui/label.h"
#include "ui/layout.h"
#include "ui/scrollbox.h"


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

#ifdef UI

		ui::RootWindow * rw = new ui::RootWindow(window_->cols(), window_->rows());
		rw->setLayout(ui::Layout::Maximized());
		rw->setBorder(ui::Border(1, 1, 1, 1));

		//rw->setLayout(ui::Layout::Horizontal());

		ui::Label* l0 = new ui::Label();
		ui::Label* l1 = new ui::Label();
		ui::Label* l2 = new ui::Label();
		ui::Label* l3 = new ui::Label();
		ui::Label* l4 = new ui::Label();
		ui::Label* l5 = new ui::Label();
		ui::Label* l6 = new ui::Label();
		ui::Label* l7 = new ui::Label();
		ui::Label* l8 = new ui::Label();
		ui::Label* l9 = new ui::Label();
		l1->setBackground(ui::Color::Magenta());
		l3->setBackground(ui::Color::Magenta());
		l5->setBackground(ui::Color::Magenta());
		l7->setBackground(ui::Color::Magenta());
		l9->setBackground(ui::Color::Magenta());
		l0->setText("This is the zero one");
		l1->setText("First");
		l2->setText("Second");
		l3->setText("Third");
		l4->setText("Fourth");
		l5->setText("Sixth");
		l6->setText("Sevneth");
		l7->setText("Eight");
		l8->setText("Ninth");
		ui::ScrollBox* sb = new ui::ScrollBox();
		sb->setLayout(ui::Layout::Horizontal());
		sb->setScrollSize(40, 100);
		sb->addChild(l0);
		sb->addChild(l1);
		sb->addChild(l2);
		sb->addChild(l3);
		sb->addChild(l4);
		sb->addChild(l5);
		sb->addChild(l6);
		sb->addChild(l7);
		sb->addChild(l8);
		sb->addChild(l9);
		rw->addChild(sb);
		window_->setTerminal(rw);
		terminal_ = rw;
		rw->repaint();
		sb->setScrollOffset(0,0);
		//sb->setLayout(ui::Layout::Vertical());

#else
		// create the terminal backend
		terminal_ = new vterm::VT100(window_->cols(), window_->rows(), pty_);
		window_->setTerminal(terminal_);
#endif
	}

} // namespace tpp