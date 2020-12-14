#pragma once

#include "ui/widgets/window.h"
#include "ui/widgets/pager.h"
#include "ui/widgets/panel.h"
#include "ui/widgets/label.h"
#include "ui/widgets/dialog.h"
#include "ui-terminal/ansi_terminal.h"
#include "ui-terminal/terminal_ui.h"
#include "tpp-lib/local_pty.h"
#include "tpp-lib/bypass_pty.h"
#include "tpp-lib/remote_files.h"

#include "../config.h"
#include "../window.h"

#include "about_box.h"

namespace tpp {

    /** New Version Dialog. 
     */
    class NewVersionDialog : public ui::Dialog::Cancel {
    public:
        explicit NewVersionDialog(std::string const & message):
            ui::Dialog::Cancel{"New Version"},
            contents_{new ui::Label{message}} {
            setBody(contents_);
        }

    private:
        Label * contents_;
    };

    /** Paste confirmation dialog. 
     */
    class PasteDialog : public ui::Dialog::YesNoCancel {
    public:

        explicit PasteDialog(std::string const & contents):
            ui::Dialog::YesNoCancel{"Are you sure you want to paste?"},
            contents_{new ui::Label{contents}} {
            setBody(contents_);
        }

        std::string const & contents() const {
            return contents_->text();
        }

    protected:

        void keyDown(KeyEvent::Payload & event) override {
            if (*event == SHORTCUT_PASTE) {
                dismiss(btnYes());
                return;
            }
            Dialog::YesNoCancel::keyDown(event);
        }

    private:
        Label * contents_;
    };     

    /** Clipboard copy confirmation dialog. 
     */
    class CopyDialog : public ui::Dialog::YesNoCancel {
    public:
        explicit CopyDialog(std::string const & contents):
            ui::Dialog::YesNoCancel{"Do you want to set clipboard to the following?"},
            contents_{new ui::Label{contents}} {
            setBody(contents_);
        }

        std::string const & contents() const {
            return contents_->text();
        }

    protected:

        void keyDown(KeyEvent::Payload & event) override {
            if (*event == SHORTCUT_COPY) {
                dismiss(btnYes());
                return;
            }
            Dialog::YesNoCancel::keyDown(event);
        }

    private:
        Label * contents_;
    };

    /** The terminal window. 
     
        
     */
    class TerminalWindow : public ui::Window {
    public:

        explicit TerminalWindow(tpp::Window * window):
            window_{window}, 
            main_{new Panel{}},
            pager_{new Pager{}} {

            window_->onClose.setHandler(&TerminalWindow::windowCloseRequest, this);
            window_->onKeyDown.setHandler(&TerminalWindow::windowKeyDown, this);

            main_->setLayout(new Layout::Column{VerticalAlign::Top});
            main_->setBackground(Color::Red);
            main_->attach(pager_);
            setContents(main_);

            Config const & config = Config::Instance();
            remoteFiles_ = new RemoteFiles(config.remoteFiles.dir());

            pager_->onPageChange.setHandler(&TerminalWindow::activeSessionChanged, this);

            window_->setRoot(this);
            versionChecker_ = std::thread{[this](){
                std::string channel = Config::Instance().version.checkChannel();
                if (channel.empty())
                    return;
                std::string newVersion = Application::Instance()->checkLatestVersion(channel);
                if (!newVersion.empty()) {
                    schedule([this, newVersion]() {
                        NewVersionDialog * d = new NewVersionDialog{STR("New version " << newVersion << " is available")};
                        showModal(d);
                    });
                }
            }};

            setFocusable(true);

            //setName("TerminalWindow");
        }

        ~TerminalWindow() override {
            versionChecker_.join();
            delete remoteFiles_;
        }

        void newSession(Config::sessions_entry const & session);


    private:
        class SessionInfo {
        public:
            std::string name;
            std::string title;
            AnsiTerminal * terminal = nullptr;
            TerminalUI<AnsiTerminal> * terminalUi = nullptr;
            bool terminateOnKeyPress = false;
            /** If true, the current session has an active notification. 
             */
            bool notification = false;
            PasteDialog * pendingPaste = nullptr;

            explicit SessionInfo(Config::sessions_entry const & session):
                name{session.name()},
                title{session.name()} {
            }

            ~SessionInfo() {
                delete terminal;
            }
        }; 

        /** The window has been requested to close. 
         
            TODO check that there are no active sessions and perhaps ask if there are whether really to exit. 
         */
        void windowCloseRequest(tpp::Window::CloseEvent::Payload & e) {
            MARK_AS_UNUSED(e);
        }

        /** Global hotkeys handling. 
         */
        void windowKeyDown(tpp::Window::KeyEvent::Payload & e) {
            // a keydown also clears any active notification in the current session, and if this is the last active notification, also the notification icon
            if (activeSession_ != nullptr) {
                if (activeSession_->notification) {
                    activeSession_->notification = false;
                    ASSERT(activeNotifications_ > 0);
                    if (--activeNotifications_ == 0)
                        window_->setIcon(tpp::Window::Icon::Default);
                }
            }
            if (*e == SHORTCUT_FULLSCREEN) {
                window_->setFullscreen(! window_->fullscreen());
            } else if (*e == SHORTCUT_SETTINGS) {
                Application::Instance()->openLocalFile(Config::GetSettingsFile(), /* edit = */ true);
            } else if (*e == SHORTCUT_ZOOM_IN || *e == SHORTCUT_ZOOM_IN_ALT) {
                if (window_->zoom() < 10)
                    window_->setZoom(window_->zoom() * 1.25);
            } else if (*e == SHORTCUT_ZOOM_OUT || *e == SHORTCUT_ZOOM_OUT_ALT) {
                if (window_->zoom() > 1)
                    window_->setZoom(std::max(1.0, window_->zoom() / 1.25));
            } else if (*e == SHORTCUT_ABOUT && ! window_->isModal()) {
                showModal(new AboutBox{});
            } else {
                return;
            }
            e.stop();
        }

        SessionInfo * sessionInfo(Widget * terminal) {
            return sessionInfo(dynamic_cast<AnsiTerminal*>(terminal));
        }

        SessionInfo * sessionInfo(AnsiTerminal * terminal) {
            ASSERT(terminal != nullptr);
            auto i = sessions_.find(terminal);
            ASSERT(i != sessions_.end());
            return (i->second);
        }

        void closeSession(SessionInfo * session) {
            UI_THREAD_ONLY;
            if (session->pendingPaste != nullptr)
                session->pendingPaste->dismiss(session->pendingPaste->btnCancel());
            sessions_.erase(session->terminal);
            pager_->removePage(session->terminalUi);
            delete session;
            // if this was the last session, close the window
            if (sessions_.empty())
                window_->requestClose();
            // otherwise focus the active session instead
            // TODO do we want this always, or should we actually check if the remove session was focused first? 
            else
                window_->setKeyboardFocus(pager_->activePage());
        }

        void sessionTitleChanged(ui::StringEvent::Payload & e) {
            SessionInfo * si = sessionInfo(e.sender());
            si->title = *e;
            if (activeSession_ == si)
                window_->setTitle(*e);
        }

        void hyperlinkOpen(ui::StringEvent::Payload & e) {
            Application::Instance()->openUrl(*e);
        }

        void hyperlinkCopy(ui::StringEvent::Payload & e) {
            Application::Instance()->setClipboard(*e);
        }

        /** Changes the icon when terminal sends notification. 
         
            Changes the icon, marks the notification flag for the terminal's session and increments the window notifications counter. 
         */
        void sessionNotification(ui::VoidEvent::Payload & e) {
            SessionInfo * si = sessionInfo(e.sender());
            // only increment the counter if current session does not have active notification enabled already
            if (si->notification == false) {
                si->notification = true;
                if (++activeNotifications_ == 1)
                    window_->setIcon(tpp::Window::Icon::Notification);
            }
        }

        void keyDown(KeyEvent::Payload & e) override {
            if (activeSession_->terminateOnKeyPress && ! e->isModifierKey()) 
                closeSession(activeSession_);
            else 
                Widget::keyDown(e);
        }

        void activeSessionChanged(ui::Event<Widget*>::Payload & e) {
            activeSession_ = *e == nullptr ? 
                nullptr : sessionInfo(dynamic_cast<TerminalUI<AnsiTerminal>*>(*e)->terminal());
            // set own background to the session's terminal background so that it propagates to the window's background
            if (activeSession_ != nullptr) {
                setBackground(activeSession_->terminal->palette().defaultBackground());
            }
        }

        void sessionPTYTerminated(ExitCodeEvent::Payload & e) {
            SessionInfo * si = sessionInfo(e.sender());
            window_->setIcon(tpp::Window::Icon::Notification);
            si->title = STR("Terminated, exit code " << *e);
            if (activeSession_ == si)
                window_->setTitle(si->title);
            Config const & config = Config::Instance();
            if (! config.renderer.window.waitAfterPtyTerminated()) {
                closeSession(activeSession_);
            } else {
                window_->setKeyboardFocus(this);
                activeSession_->terminateOnKeyPress = true;
            }
        }

        void terminalSetClipboard(StringEvent::Payload & e) {
            switch (Config::Instance().sequences.allowClipboardUpdate()) {
                case config::AllowClipboardUpdate::Deny:
                    break;
                case config::AllowClipboardUpdate::Allow:
                    setClipboard(*e);
                    break;
                case config::AllowClipboardUpdate::Ask: {
                    CopyDialog * d = new CopyDialog(*e);
                    d->onDismiss.setHandler([d, this](ui::Event<Widget*>::Payload & e) {
                        if (*e == d->btnYes())
                            setClipboard(d->contents());
                    });
                    showModal(d);
                    break;
                }
            }
        }

        void terminalPaste(StringEvent::Payload & e) {
            SessionInfo * si = sessionInfo(e.sender());
            switch (Config::Instance().sequences.confirmPaste()) {
                case config::ConfirmPaste::Never:
                    si->terminal->pasteContents(*e);
                    return;
                case config::ConfirmPaste::Multiline:
                    if ((*e).find('\n') == std::string::npos) {
                        si->terminal->pasteContents(*e);
                        return;
                    }
                    [[fallthrough]]; // fallthrough to always
                case config::ConfirmPaste::Always:
                    break;
            }
            if (si->pendingPaste != nullptr)
                si->pendingPaste->cancel();
            si->pendingPaste = new PasteDialog(*e);
            si->pendingPaste->onDismiss.setHandler([si](ui::Event<Widget*>::Payload & e) {
                if (*e == si->pendingPaste->btnYes())
                    si->terminal->pasteContents(si->pendingPaste->contents());
                si->pendingPaste = nullptr;
            });
            showModal(si->pendingPaste);
        }

        void terminalKeyDown(KeyEvent::Payload & e) {
            SessionInfo * si = sessionInfo(e.sender());
            if (*e == SHORTCUT_PASTE) {
                si->terminal->requestClipboardPaste();
            } else {
                return;
            }
            e.stop();
        }

        void terminalTppSequence(TppSequenceEvent::Payload & event) {
            SessionInfo * si = sessionInfo(event.sender());
            try {
                switch (event->kind) {
                    case tpp::Sequence::Kind::GetCapabilities:
                        si->terminal->pty()->send(tpp::Sequence::Capabilities{1});
                        break;
                    case tpp::Sequence::Kind::OpenFileTransfer: {
                        Sequence::OpenFileTransfer req(event->payloadStart, event->payloadEnd);
                        si->terminal->pty()->send(remoteFiles_->openFileTransfer(req));
                        break;
                    }
                    case tpp::Sequence::Kind::Data: {
                        Sequence::Data data{event->payloadStart, event->payloadEnd};
                        remoteFiles_->transfer(data);
                        // make sure the UI thread remains responsive
                        window_->yieldToUIThread();
                        break;
                    }
                    case tpp::Sequence::Kind::GetTransferStatus: {
                        Sequence::GetTransferStatus req{event->payloadStart, event->payloadEnd};
                        si->terminal->pty()->send(remoteFiles_->getTransferStatus(req));
                        break;
                    }
                    case tpp::Sequence::Kind::ViewRemoteFile: {
                        Sequence::ViewRemoteFile req{event->payloadStart, event->payloadEnd};
                        RemoteFiles::File * f = remoteFiles_->get(req.id());
                        if (f == nullptr) {
                            si->terminal->pty()->send(Sequence::Nack{req, "No such file"});
                        } else if (! f->ready()) {
                            si->terminal->pty()->send(Sequence::Nack(req, "File not transferred"));
                        } else {
                            // send the ack first in case there are local issues with the opening
                            si->terminal->pty()->send(Sequence::Ack{req, req.id()});
                            Application::Instance()->openLocalFile(f->localPath(), false);
                        }
                        break;
                    }
                    default:
                        LOG() << "Unknown sequence";
                        break;
                }
            } catch (std::exception const & e) {
                showError(e.what());
            }
        }


        tpp::Window * window_;

        ui::Panel * main_;
        ui::Pager * pager_;

        std::unordered_map<AnsiTerminal *, SessionInfo *> sessions_; 

        SessionInfo * activeSession_ = nullptr;
        unsigned activeNotifications_ = 0;

        RemoteFiles * remoteFiles_;

        std::thread versionChecker_;

    };

} // namespace tpp
