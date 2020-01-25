#pragma once 

#include "ui/widgets/label.h"
#include "ui/root_window.h"
#include "ui/builders.h"

#include "ui-terminal/terminal.h"

#include "../config.h"
#include "../remote_files.h"

#include "about_box.h"


namespace tpp {

    /** A window displaying single session. 
      
        The session consists of a terminal attached to a PTY and corresponding ui, such as the information about the program, etc. 
     */
    class Session : public ui::RootWindow {
    public:
        Session(ui::Terminal::PTY * pty, ui::Terminal::Palette * palette):
            pty_(pty),
            closeOnKeyDown_(false),
            remoteFiles_(Config::Instance().session.remoteFiles.dir()) {
            Config const & config = Config::Instance();
            using namespace ui;
            Create(this) 
                /*
                << Layout::Horizontal()
                << (Create(new ui::Label()))
                */
                << Layout::Maximized
                << (Create(terminal_ = new ui::Terminal(config.session.cols(), config.session.rows(), palette, pty, config.renderer.fps()))
                    << FocusIndex(0)
                    << FocusStop(true)
                    << HistorySizeLimit(config.session.historyLimit())
                    << BoldIsBright(config.session.sequences.boldIsBright())
                );

            setBackgroundColor(palette->defaultBackground());

            terminal_->onTitleChange.setHandler(&Session::terminalTitleChanged, this);
            terminal_->onNotification.setHandler(&Session::terminalNotification, this);
            terminal_->onPTYTerminated.setHandler(&Session::ptyTerminated, this);
            terminal_->onTppNewFile.setHandler(&Session::newRemoteFile, this);
            terminal_->onTppData.setHandler(&Session::remoteData, this);
            terminal_->onTppTransferStatus.setHandler(&Session::transferStatus, this);
            terminal_->onTppOpenFile.setHandler(&Session::openRemoteFile, this);
            terminal_->onInputError.setHandler(&Session::terminalInputError, this);

            terminal_->setCursor(config.session.cursor());
            about_ = new AboutBox();

            focusWidget(terminal_, true);
            if (! config.session.log().empty()) {
                logFile_.open(config.session.log());
                terminal_->onInput.setHandler(&Session::terminalInputProcessed, this);
            }

        }

        ~Session() override {
            hideModalWidget();
            delete about_;
        }

    protected:

        void attachRenderer(ui::Renderer * renderer) override {
            RootWindow::attachRenderer(renderer);
            // start the terminal once all configuration is done
            terminal_->start();
        }

    private:

        void terminalTitleChanged(ui::Event<std::string>::Payload & e) {
            setTitle(*e);
        }

        // TODO check that file exists actually
        void newRemoteFile(ui::Event<ui::TppNewFileEvent>::Payload & e) {
            RemoteFile * f = remoteFiles_.newFile(e->request.hostname, e->request.filename, e->request.remotePath, e->request.size);
            e->response.fileId = f->id();
        }

        void remoteData(ui::Event<ui::TppDataEvent>::Payload & e) {
            RemoteFile * f = remoteFiles_.get(e->fileId);
            if (e->offset == f->transmittedBytes())
                f->appendData(e->data.c_str(), e->data.size());
            else
                LOG() << "error";
        }

        void transferStatus(ui::Event<ui::TppTransferStatusEvent>::Payload & e) {
            RemoteFile * f = remoteFiles_.get(e->request.fileId);
            e->response.fileId = e->request.fileId;
            e->response.transmittedBytes = f->transmittedBytes();
        }

        /** Opens the remote file which has been copied to the given local path.
         */
        void openRemoteFile(ui::Event<ui::TppOpenFileEvent>::Payload & e) {
            RemoteFile * f = remoteFiles_.get(e->fileId);
            if (f->available()) {
                Application::Open(f->localPath());
            } else {
                Application::Alert(STR("Incomplete file " << f->localPath() << " received. Unable to open"));
            }
        }

        void terminalNotification(ui::Event<void>::Payload & e) {
            MARK_AS_UNUSED(e);
            setIcon(Icon::Notification);
        }

        void ptyTerminated(ui::Event<helpers::ExitCode>::Payload & e) {
            // disable the terminal
            terminal_->setEnabled(false);
            if (*e != EXIT_SUCCESS || Config::Instance().session.waitAfterPtyTerminated()) {
                setTitle(STR("Attached process terminated (code " << *e << ") - press a key to exit"));
                setIcon(Icon::Notification);
                // close the window upon next key down
                closeOnKeyDown_ = true;
            } else {
                closeRenderer();
            }
        }

        void terminalInputProcessed(ui::Event<ui::InputProcessedEvent>::Payload & e) {
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
                    about_->show(this);
                else
                    ui::RootWindow::keyDown(k);
            }
        }

        void terminalInputError(ui::Event<ui::InputErrorEvent>::Payload & e) {
            Application::Alert(e->error);
        }

        ui::Terminal::PTY * pty_;
        ui::Terminal * terminal_;
        AboutBox * about_;

        std::ofstream logFile_;

        bool closeOnKeyDown_;

        RemoteFiles remoteFiles_;

    }; // tpp::Session

} // namespace tpp

