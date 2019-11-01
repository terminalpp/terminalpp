#pragma once 

#include "ui/widgets/label.h"
#include "ui/root_window.h"
#include "ui/builders.h"

#include "tpp-widget/terminalpp.h"

#include "../config.h"
#include "../remote_files.h"

#include "about_box.h"


namespace tpp {

    /** A window displaying single session. 
      
        The session consists of a terminal attached to a PTY and corresponding ui, such as the information about the program, etc. 
     */
    class Session : public ui::RootWindow {
    public:
        Session(ui::PTY * pty, ui::TerminalPP::Palette * palette):
            pty_(pty),
            closeOnKeyDown_(false) {
            Config const & config = Config::Instance();
            using namespace ui;
            Create(this) 
                /*
                << Layout::Horizontal()
                << (Create(new ui::Label()))
                */
                << Layout::Maximized()
                << (Create(terminal_ = new TerminalPP(config.sessionCols(), config.sessionRows(), palette, pty, config.rendererFps()))
                    << FocusIndex(0)
                    << FocusStop(true)
                    << HistorySizeLimit(config.sessionHistoryLimit())
                    << BoldIsBright(config.sessionSequencesBoldIsBright())
                    << OnTitleChange(CreateHandler<StringEvent, Session, &Session::terminalTitleChanged>(this))
                    << OnNotification(CreateHandler<VoidEvent, Session, &Session::terminalNotification>(this))
                    << OnPTYTerminated(CreateHandler<ExitCodeEvent, Session, &Session::ptyTerminated>(this))
                    << OnTppNewFile(CreateHandler<TppNewFileEvent, Session, &Session::newRemoteFile>(this))
                    << OnTppData(CreateHandler<TppDataEvent, Session, &Session::remoteData>(this))
                    << OnTppTransferStatus(CreateHandler<TppTransferStatusEvent, Session, &Session::transferStatus>(this))
                    << OnTppOpenFile(CreateHandler<TppOpenFileEvent, Session, &Session::openRemoteFile>(this))
                    << OnInputError(CreateHandler<InputErrorEvent, Session, &Session::terminalInputError>(this))
                    //<< OnLineScrolledOut(CreateHandler<LineScrollEvent, Session, &Session::lineScrolledOut>(this))
                )
                << (Create(about_ = new AboutBox())
                  << Visible(false)
                  << OnDismissed(CreateHandler<VoidEvent, Session, &Session::aboutBoxDismissed>(this))
                )
                ;

            focusWidget(terminal_, true);
            if (! config.logFile().empty()) {
                logFile_.open(config.logFile());
                terminal_->onInput += CreateHandler<InputProcessedEvent, Session, &Session::terminalInputProcessed>(this);
            }
        }

    private:

        void aboutBoxDismissed(ui::VoidEvent & e) {
            MARK_AS_UNUSED(e);
            terminal_->setFocused(true);
        }

        void terminalTitleChanged(ui::StringEvent & e) {
            setTitle(*e);
        }

        // TODO check that file exists actually
        void newRemoteFile(ui::TppNewFileEvent & e) {
            RemoteFile * f = remoteFiles_.newFile(e->request.hostname, e->request.filename, e->request.remotePath, e->request.size);
            e->response.fileId = f->id();
        }

        void remoteData(ui::TppDataEvent & e) {
            RemoteFile * f = remoteFiles_.get(e->fileId);
            if (e->offset == f->transmittedBytes())
                f->appendData(e->data.c_str(), e->data.size());
            else
                LOG << "error";
        }

        void transferStatus(ui::TppTransferStatusEvent & e) {
            RemoteFile * f = remoteFiles_.get(e->request.fileId);
            e->response.fileId = e->request.fileId;
            e->response.transmittedBytes = f->transmittedBytes();
        }

        /** Opens the remote file which has been copied to the given local path.
         */
        void openRemoteFile(ui::TppOpenFileEvent & e) {
            RemoteFile * f = remoteFiles_.get(e->fileId);
            if (f->available()) {
                Application::Open(f->localPath());
            } else {
                Application::Alert(STR("Incomplete file " << f->localPath() << " received. Unable to open"));
            }
        }

        void terminalNotification(ui::VoidEvent & e) {
            MARK_AS_UNUSED(e);
            setIcon(Icon::Notification);
        }

        void ptyTerminated(ui::ExitCodeEvent & e) {
            // disable the terminal
            terminal_->setEnabled(false);
            if (*e != EXIT_SUCCESS || Config::Instance().sessionWaitAfterPtyTerminated()) {
                setTitle(STR("Attached process terminated (code " << *e << ") - press a key to exit"));
                setIcon(Icon::Notification);
                // close the window upon next key down
                closeOnKeyDown_ = true;
            } else {
                closeRenderer();
            }
        }

        void terminalInputProcessed(ui::InputProcessedEvent & e) {
            logFile_.write(e->buffer, e->size);
        }

        void mouseDown(int col, int row, ui::MouseButton button, ui::Key modifiers) override {
            setIcon(Icon::Default);
            ui::RootWindow::mouseDown(col, row, button, modifiers);
        }

        void mouseWheel(int col, int row, int by, ui::Key modifiers) override {
            setIcon(Icon::Default);
            ui::RootWindow::mouseWheel(col, row, by, modifiers);
        }

        void keyChar(helpers::Char c) override {
            setIcon(Icon::Default);
            ui::RootWindow::keyChar(c);
        }

        void keyDown(ui::Key k) override {
            if (closeOnKeyDown_) {
                closeRenderer();
            } else {
                setIcon(Icon::Default);
                if (k == SHORTCUT_ABOUT)
                    about_->show();
                else
                    ui::RootWindow::keyDown(k);
            }
        }

        void terminalInputError(ui::InputErrorEvent & e) {
            Application::Alert(e->error);
        }

        ui::PTY * pty_;
        ui::TerminalPP * terminal_;
        AboutBox * about_;

        std::ofstream logFile_;

        bool closeOnKeyDown_;

        RemoteFiles remoteFiles_;

    }; // tpp::Session

} // namespace tpp

