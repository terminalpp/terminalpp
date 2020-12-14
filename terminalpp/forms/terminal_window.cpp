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
        si->terminal->setBoldIsBright(config.sequences.boldIsBright());
        si->terminal->setDisplayBold(config.sequences.displayBold());
        si->terminal->setCursor(session.cursor());
        si->terminal->setInactiveCursorColor(session.cursor.inactiveColor());
        si->terminal->setAllowCursorChanges(config.sequences.allowCursorChanges());
        si->terminal->setAllowOSCHyperlinks(config.sequences.allowOSCHyperlinks());
        si->terminal->setDetectHyperlinks(config.sequences.detectHyperlinks());
        si->terminal->setNormalHyperlinkStyle(config.renderer.hyperlinks.normal());
        si->terminal->setActiveHyperlinkStyle(config.renderer.hyperlinks.active());
        // wrap the terminal in ui widget
        si->terminalUi = new TerminalUI{si->terminal};
        si->terminalUi->setMaxHistoryRows(config.renderer.window.historyLimit());
        // register the session and set it as active page
        TerminalUI<AnsiTerminal> * t = si->terminalUi;
        sessions_.insert(std::make_pair(t->terminal(), si.release()));
        pager_->setActivePage(t);
        window_->setKeyboardFocus(t->terminal());
        // add terminal events (so that they are called only *after* the session is registered)
        t->terminal()->onPTYTerminated.setHandler(&TerminalWindow::sessionPTYTerminated, this);
        t->terminal()->onTitleChange.setHandler(&TerminalWindow::sessionTitleChanged, this);
        t->terminal()->onClipboardSetRequest.setHandler(&TerminalWindow::terminalSetClipboard, this);
        t->terminal()->onPaste.setHandler(&TerminalWindow::terminalPaste, this);
        t->terminal()->onNotification.setHandler(&TerminalWindow::sessionNotification, this);
        t->terminal()->onTppSequence.setHandler(&TerminalWindow::terminalTppSequence, this);
        t->terminal()->onKeyDown.setHandler(&TerminalWindow::terminalKeyDown, this);
        t->terminal()->onHyperlinkOpen.setHandler(&TerminalWindow::hyperlinkOpen, this);
        t->terminal()->onHyperlinkCopy.setHandler(&TerminalWindow::hyperlinkCopy, this);
    }

} // namespace tpp

