#include "terminal_window.h"


namespace tpp {

    using namespace ui;

    void TerminalWindow::newSession(Config::sessions_entry const & session) {
        Config const & config = Config::Instance();
        std::unique_ptr<SessionInfo> si{new SessionInfo{session}};
        // create the pty
        PTYMaster * pty = nullptr;
        // sets the working directory of the command to the specified working directory of the session,
        Command cmd = session.command();
        cmd.setWorkingDirectory(session.workingDirectory());
#if (ARCH_WINDOWS)
        if (session.pty() != "bypass") 
            pty = new LocalPTYMaster(cmd);
        else
            pty = new BypassPTYMaster(cmd);
#else
        pty = new LocalPTYMaster{cmd};
#endif
        // and the terminal
        si->terminal = new AnsiTerminal{pty, session.palette()};
        si->terminal->setMaxHistoryRows(config.renderer.window.historyLimit());
        si->terminal->setBoldIsBright(config.sequences.boldIsBright());
        si->terminal->setDisplayBold(config.sequences.displayBold());
        si->terminal->setInactiveCursorColor(session.cursor.inactiveColor());
        si->terminal->setAllowOSCHyperlinks(config.sequences.allowOSCHyperlinks());
        si->terminal->setDetectHyperlinks(config.sequences.detectHyperlinks());
        si->terminal->setCursor(session.cursor());
        // register the session and set it as active page
        AnsiTerminal * t = si->terminal;
        sessions_.insert(std::make_pair(t, si.release()));
        pager_->setActivePage(t);
        window_->setKeyboardFocus(t);
        // add terminal events (so that they are called only *after* the session is registered)
        t->onPTYTerminated.setHandler(&TerminalWindow::sessionPTYTerminated, this);
        t->onTitleChange.setHandler(&TerminalWindow::sessionTitleChanged, this);
        t->onClipboardSetRequest.setHandler(&TerminalWindow::terminalSetClipboard, this);
        t->onPaste.setHandler(&TerminalWindow::terminalPaste, this);
        t->onNotification.setHandler(&TerminalWindow::sessionNotification, this);
        t->onTppSequence.setHandler(&TerminalWindow::terminalTppSequence, this);
        t->onKeyDown.setHandler(&TerminalWindow::terminalKeyDown, this);
        t->onHyperlinkOpen.setHandler(&TerminalWindow::hyperlinkOpen, this);
        t->onHyperlinkCopy.setHandler(&TerminalWindow::hyperlinkCopy, this);
    }

} // namespace tpp

