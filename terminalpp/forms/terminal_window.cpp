#include "terminal_window.h"

namespace tpp {

    void TerminalWindow::newSession(Config::sessions_entry const & session) {
        Config const & config = Config::Instance();
        std::unique_ptr<SessionInfo> si{new SessionInfo{session}};
        // create the pty
#if (ARCH_WINDOWS)
        if (session.pty() != "bypass") 
            si->pty = new LocalPTYMaster(session.command());
        else
            si->pty = new BypassPTYMaster(session.command());
#else
        si->pty = new LocalPTYMaster{session.command()};
#endif
        // and the terminal
        si->terminal = new AnsiTerminal{si->pty, & si->palette, pager_->width(), pager_->height()};
        si->terminal->setHistoryLimit(config.renderer.window.historyLimit());
        si->terminal->setDefaultCursor(session.cursor());
        si->terminal->setInactiveCursorColor(session.cursor.inactiveColor());
        si->terminal->setBoldIsBright(config.sequences.boldIsBright());
        // register the session and set it as active page
        AnsiTerminal * t = si->terminal;
        sessions_.insert(std::make_pair(t, si.release()));
        pager_->setActivePage(t);
        window_->setKeyboardFocus(t);
        // add terminal events (so that they are called only *after* the session is registered)
        /*
        si->terminal->onPTYTerminated.setHandler(&TerminalWindow::terminalPTYTerminated, this);
        si->terminal->onTitleChange.setHandler(&TerminalWindow::terminalTitleChanged, this);
        si->terminal->onNotification.setHandler(&TerminalWindow::terminalNotification, this);
        si->terminal->onKeyDown.setHandler(&TerminalWindow::terminalKeyDown, this);
        si->terminal->onMouseMove.setHandler(&TerminalWindow::terminalMouseMove, this);
        si->terminal->onMouseDown.setHandler(&TerminalWindow::terminalMouseDown, this);
        si->terminal->onMouseUp.setHandler(&TerminalWindow::terminalMouseUp, this);
        si->terminal->onMouseWheel.setHandler(&TerminalWindow::terminalMouseWheel, this);
        si->terminal->onSetClipboard.setHandler(&TerminalWindow::terminalSetClipboard, this);
        si->terminal->onTppSequence.setHandler(&TerminalWindow::terminalTppSequence, this);
        */
    }

} // namespace tpp