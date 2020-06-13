#pragma once 

#include "../config.h"

#include "ui/widgets/panel.h"
#include "ui/widgets/button.h"
#include "ui/widgets/label.h"
#include "ui/widgets/dialog.h"
#include "ui/layouts/maximize.h"
#include "ui/layouts/column.h"
#include "ui/layouts/row.h"
#include "ui-terminal/ansi_terminal.h"
#include "tpp-lib/local_pty.h"
#include "tpp-lib/bypass_pty.h"
#include "tpp-lib/remote_files.h"

#include "about_box.h"


namespace tpp {

    /** Error dialog. 
     */
    class ErrorDialog : public ui::Dialog::Cancel {
    public:
        ErrorDialog(std::string const & message):
            Dialog::Cancel{"Error", /* deleteOnDismiss */ true},
            contents_{new Label{message}} {
            setBody(contents_);
        }

    private:
        Label * contents_;
    };

    /** New Version Dialog. 
     */
    class NewVersionDialog : public ui::Dialog::Cancel {
    public:
        NewVersionDialog(std::string const & message):
            Dialog::Cancel{"New Version", /* deleteOnDismiss */ true},
            contents_{new Label{message}} {
            setBody(contents_);
            setSemanticStyle(SemanticStyle::Info);
        }

    private:
        Label * contents_;
    };


    /** Paste confirmation dialog. 
     */
    class PasteDialog : public ui::Dialog::YesNoCancel {
    public:

        static PasteDialog * CreateFor(std::string const & contents) {
            Config const & config = Config::Instance();
            if (config.sequences.confirmPaste() == "never")
                return nullptr;
            if (config.sequences.confirmPaste() == "multiline" && contents.find('\n') == std::string::npos)
                return nullptr;
            return new PasteDialog(contents);
        }

        std::string const & contents() const {
            return contents_->text();
        }

    protected:

        PasteDialog(std::string const & contents):
            Dialog::YesNoCancel{"Are you sure you want to paste?", /* deleteOnDismiss */ true},
            contents_{new Label{contents}} {
            setBody(contents_);
        }

        void keyDown(UIEvent<Key>::Payload & event) override {
            if (*event == SHORTCUT_PASTE) {
                dismiss(btnYes_);
                return;
            }
            Dialog::YesNoCancel::keyDown(event);
        }

    private:
        Label * contents_;
    }; 

    /** The terminal session. 
     */
    class Session : public ui::CustomPanel, public ui::AutoScroller<Session> {
    public:

        Session(Window * window, Config::sessions_entry const & session):
            window_{window},
            terminateOnKeyPress_{false},
            palette_{session.palette()} {
        	Config const & config = Config::Instance();
            window_->setRootWidget(this);
#if (ARCH_WINDOWS)
            if (session.pty() != "bypass") 
                pty_ = new LocalPTYMaster(session.command());
            else
                pty_ = new BypassPTYMaster(session.command());
#else
            pty_ = new LocalPTYMaster{session.command()};
#endif

            mainWindow_ = new PublicContainer{new ColumnLayout{VerticalAlign::Top}};
            notifications_ = new ModalPane();
            notifications_->setModal(false);
            notifications_->setHeightHint(SizeHint::Auto());
            mainWindow_->add(notifications_);

            terminal_ = new AnsiTerminal{pty_, & palette_, width(), height()};
            terminal_->setHistoryLimit(config.renderer.window.historyLimit());
            terminal_->setDefaultCursor(session.cursor());
            terminal_->setInactiveCursorColor(session.cursor.inactiveColor());
            terminal_->setBoldIsBright(config.sequences.boldIsBright());
            //terminal_->setHeightHint(SizeHint::Percentage(75));
            terminal_->onPTYTerminated.setHandler(&Session::terminalPTYTerminated, this);
            terminal_->onTitleChange.setHandler(&Session::terminalTitleChanged, this);
            terminal_->onNotification.setHandler(&Session::terminalNotification, this);
            terminal_->onKeyDown.setHandler(&Session::terminalKeyDown, this);
            terminal_->onMouseMove.setHandler(&Session::terminalMouseMove, this);
            terminal_->onMouseDown.setHandler(&Session::terminalMouseDown, this);
            terminal_->onMouseUp.setHandler(&Session::terminalMouseUp, this);
            terminal_->onMouseWheel.setHandler(&Session::terminalMouseWheel, this);
            terminal_->onSetClipboard.setHandler(&Session::terminalSetClipboard, this);
            terminal_->onTppSequence.setHandler(&Session::terminalTppSequence, this);
            setLayout(new MaximizeLayout());
            //setLayout(new ColumnLayout(VerticalAlign::Bottom));
            //setBorder(Border{Color::Blue}.setAll(Border::Kind::Thick));
            //add(terminal_);


            terminal_->setHeightHint(SizeHint::Percentage(100));
            mainWindow_->add(terminal_);
            add(mainWindow_);

            modalPane_ = new ModalPane();
            //modalPane_->setLayout(new ColumnLayout{VerticalAlign::Bottom});
            //modalPane_->setHeightHint(SizeHint::Auto());
            add(modalPane_);

            // set the session itself as focusable so that it can accept keyboard events if no-one else does
            setFocusable(true);



            //window_->setKeyboardFocus(terminal_);
            remoteFiles_ = new RemoteFiles(config.remoteFiles.dir());

            if (config.renderer.window.fullscreen())
                window_->setFullscreen(true);
            
            versionChecker_ = std::thread{[this](){
                std::string channel = Config::Instance().version.checkChannel();
                // don't check if empty channel
                if (channel.empty())
                    return;
                std::string newVersion = Application::Instance()->checkLatestVersion(channel);
                if (!newVersion.empty()) {
                    sendEvent([this, newVersion]() {
                        NewVersionDialog * d = new NewVersionDialog{STR("New version " << newVersion << " is available")};
                        notifications_->add(d);
                    });
                }
            }};
        }

        ~Session() override {
            versionChecker_.join();
            delete remoteFiles_;
        }

    protected:

        void showError(std::string const & error) {
            // TODO log this as well
            sendEvent([this, error]() {
                ErrorDialog * e = new ErrorDialog{error};
                modalPane_->add(e);
            });
        }

        void keyDown(UIEvent<Key>::Payload & event) override {
            if (terminateOnKeyPress_) 
                window_->requestClose();
            else 
                Widget::keyDown(event);
        }

        bool autoScrollStep(Point by) override {
            return terminal_->scrollBy(by);
        }

        void terminalPTYTerminated(UIEvent<ExitCode>::Payload & e) {
            window_->setIcon(Window::Icon::Notification);
            window_->setTitle(STR("Terminated, exit code " << *e));
            Config const & config = Config::Instance();
            if (! config.renderer.window.waitAfterPtyTerminated())
                window_->requestClose();
            else 
                terminateOnKeyPress_ = true;
        }        

        void terminalTitleChanged(UIEvent<std::string>::Payload & e) {
            window_->setTitle(*e);
        }

        void terminalNotification(UIEvent<void>::Payload & e) {
            MARK_AS_UNUSED(e);
            window_->setIcon(Window::Icon::Notification);
        }

        void terminalKeyDown(UIEvent<Key>::Payload & e) {
            if (window_->icon() != Window::Icon::Default)
                window_->setIcon(Window::Icon::Default);
            // trigger paste event for ourselves so that the paste can be intercepted
            if (*e == SHORTCUT_PASTE) {
                requestClipboard();
                e.stop();
            } else if (*e == SHORTCUT_ABOUT) {
                AboutBox * ab = new AboutBox();
                modalPane_->add(ab);
                e.stop();
            } else if (*e == SHORTCUT_SETTINGS) {
                Application::Instance()->openLocalFile(Config::GetSettingsFile(), /* edit */ true); 
            }
        }

        void terminalMouseMove(UIEvent<MouseMoveEvent>::Payload & event) {
            if (terminal_->mouseCaptured())
                return;
            if (terminal_->updatingSelection()) {
                if (event->coords.y() < 0 || event->coords.y() >= terminal_->height())
                    startAutoScroll(Point{0, event->coords.y() < 0 ? -1 : 1});
                else
                    stopAutoScroll();
            }
        }

        void terminalMouseDown(UIEvent<MouseButtonEvent>::Payload & event) {
            if (terminal_->mouseCaptured())
                return;
            if (event->modifiers == 0) {
                if (event->button == MouseButton::Left) {
                    terminal_->startSelectionUpdate(event->coords);
                } else if (event->button == MouseButton::Wheel) {
                    requestSelection(); 
                } else if (event->button == MouseButton::Right && ! terminal_->selection().empty()) {
                    setClipboard(terminal_->getSelectionContents());
                    terminal_->clearSelection();
                } else {
                    return;
                }
            }
            event.stop();
        }

        void terminalMouseUp(UIEvent<MouseButtonEvent>::Payload & event) {
            if (terminal_->mouseCaptured())
                return;
            if (event->modifiers == 0) {
                if (event->button == MouseButton::Left) 
                    terminal_->endSelectionUpdate();
                else
                    return;
            }
            event.stop();
        }

        void terminalMouseWheel(UIEvent<MouseWheelEvent>::Payload & event) {
            if (terminal_->mouseCaptured())
                return;
            //if (state_.buffer.historyRows() > 0) {
                if (event->by > 0)
                    terminal_->scrollBy(Point{0, -1});
                else 
                    terminal_->scrollBy(Point{0, 1});
                event.stop();
            //}
        }

        void terminalSetClipboard(UIEvent<std::string>::Payload & event) {
            setClipboard(*event);
        }

        void terminalTppSequence(UIEvent<TppSequenceEvent>::Payload & event) {
            try {
                switch (event->kind) {
                    case tpp::Sequence::Kind::GetCapabilities:
                        pty_->send(tpp::Sequence::Capabilities{1});
                        break;
                    case tpp::Sequence::Kind::OpenFileTransfer: {
                        Sequence::OpenFileTransfer req(event->payloadStart, event->payloadEnd);
                        pty_->send(remoteFiles_->openFileTransfer(req));
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
                        pty_->send(remoteFiles_->getTransferStatus(req));
                        break;
                    }
                    case tpp::Sequence::Kind::ViewRemoteFile: {
                        Sequence::ViewRemoteFile req{event->payloadStart, event->payloadEnd};
                        RemoteFiles::File * f = remoteFiles_->get(req.id());
                        if (f == nullptr) {
                            pty_->send(Sequence::Nack{req, "No such file"});
                        } else if (! f->ready()) {
                            pty_->send(Sequence::Nack(req, "File not transferred"));
                        } else {
                            // send the ack first in case there are local issues with the opening
                            pty_->send(Sequence::Ack{req, req.id()});
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

        void paste(UIEvent<std::string>::Payload & e) override {
            // determine whether to show the dialog, or paste immediately
            PasteDialog * pd = PasteDialog::CreateFor(*e);
            if (pd == nullptr) {
                terminal_->paste(*e);
            } else {
                pd->onDismiss.setHandler([this, pd](UIEvent<Widget*>::Payload & e) {
                    //modalPane_->dismiss(pd);
                    if (*e == pd->btnYes())
                        terminal_->paste(pd->contents());
                });
                modalPane_->add(pd);
            }
        }

    private:

        /** The window in which the session is rendered.
         */
        Window * window_;
        ModalPane * modalPane_;
        PublicContainer * mainWindow_;
        ModalPane * notifications_;
        
        bool terminateOnKeyPress_;

        AnsiTerminal::Palette palette_;
        AnsiTerminal * terminal_;
        PTYMaster * pty_;

        RemoteFiles * remoteFiles_;

        std::thread versionChecker_;

    }; 

}
