#include "terminal_window.h"


namespace tpp {

    using namespace ui;

    void TerminalWindow::newSession(Config::sessions_entry const & session) {
        Config const & config = Config::Instance();
        std::unique_ptr<SessionInfo> si{new SessionInfo{session}};
        // create the pty
        PTYMaster * pty = nullptr;
#if (ARCH_WINDOWS)
        if (session.pty() != "bypass") 
            pty = new LocalPTYMaster(session.command());
        else
            pty = new BypassPTYMaster(session.command());
#else
        pty = new LocalPTYMaster{session.command()};
#endif
        // and the terminal
        si->terminal = new AnsiTerminal{pty, session.palette()};
        /*
        si->terminal->setHistoryLimit(config.renderer.window.historyLimit());
        si->terminal->setDefaultCursor(session.cursor());
        si->terminal->setInactiveCursorColor(session.cursor.inactiveColor());
        si->terminal->setBoldIsBright(config.sequences.boldIsBright());
        */
        // register the session and set it as active page
        AnsiTerminal * t = si->terminal;
        sessions_.insert(std::make_pair(t, si.release()));
        pager_->setActivePage(t);
        window_->setKeyboardFocus(t);
        // add terminal events (so that they are called only *after* the session is registered)
        /*
        t->onPTYTerminated.setHandler(&TerminalWindow::sessionPTYTerminated, this);
        t->onTitleChange.setHandler(&TerminalWindow::sessionTitleChanged, this);
        t->onNotification.setHandler(&TerminalWindow::sessionNotification, this);
        t->onKeyDown.setHandler(&TerminalWindow::terminalKeyDown, this);
        t->onMouseMove.setHandler(&TerminalWindow::terminalMouseMove, this);
        t->onMouseDown.setHandler(&TerminalWindow::terminalMouseDown, this);
        t->onMouseUp.setHandler(&TerminalWindow::terminalMouseUp, this);
        t->onMouseWheel.setHandler(&TerminalWindow::terminalMouseWheel, this);
        t->onSetClipboard.setHandler(&TerminalWindow::terminalSetClipboard, this);
        */
        t->onTppSequence.setHandler(&TerminalWindow::terminalTppSequence, this);
    }

} // namespace tpp

