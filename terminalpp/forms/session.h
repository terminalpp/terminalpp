#pragma once 

#include "ui/widgets/label.h"
#include "ui/widgets/panel.h"
#include "ui/root_window.h"
#include "ui/builders.h"

#include "ui-terminal/terminal.h"

#include "../config.h"
#include "../remote_files.h"

#include "about_box.h"

#include "ui2/widgets/panel.h"
#include "ui-terminal/ansi-terminal.h"
#include "ui-terminal/bypass_pty.h"


namespace tpp2 {

    /** The terminal session. 
     */

    // TODO fix container's add method
    class Session : public ui2::Panel {
    public:

        Session(Window * window):
            window_{window},
            palette_{AnsiTerminal::Palette::XTerm256()} {
            window_->setRootWidget(this);
        	Config const & config = Config::Instance();
            terminal_ = new AnsiTerminal{& palette_, width(), height()};
            terminal_->onTitleChange.setHandler(&Session::terminalTitleChanged, this);
            terminal_->onNotification.setHandler(&Session::terminalNotification, this);
            terminal_->onKeyDown.setHandler(&Session::terminalKeyDown, this);
            add(terminal_);
            pty_ = new ui2::BypassPTY{terminal_, config.session.command()};
            window_->setKeyboardFocus(terminal_);
        }

    protected:
        void terminalTitleChanged(Event<std::string>::Payload & e) {
            window_->setTitle(*e);
        }

        void terminalNotification(Event<void>::Payload & e) {
            MARK_AS_UNUSED(e);
            window_->setIcon(Window::Icon::Notification);
        }

        void terminalKeyDown(Event<Key>::Payload & e) {
            MARK_AS_UNUSED(e);
            if (window_->icon() != Window::Icon::Default)
                window_->setIcon(Window::Icon::Default);
        }

    private:

        /** The window in which the session is rendered.
         */
        Window * window_;

        AnsiTerminal::Palette palette_;
        AnsiTerminal * terminal_;
        PTY * pty_;
    }; 

}

namespace tpp {

    /** An alert displayed when pasting into the terminal. 
     */
    class PasteAlert : public ui::Panel, public ui::Modal<PasteAlert> {
    public:
        PasteAlert() {
            showHeader_ = true;
            headerFont_.setBold();
            headerColor_ = ui::Color::White;
            headerBackground_ = ui::Color{255,0,0};
            headerText_ = "Are you sure you want to paste this?";
            setBorder(ui::Border::Thin(ui::Color::Red));
            setBackground(ui::Color{96, 0, 0});
            setWidthHint(ui::Layout::SizeHint::Auto());
            setHeightHint(ui::Layout::SizeHint::Fixed());
            setLayout(ui::Layout::Maximized);
            attachChild(contents_ = new ui::Label());
        }
        
        void show(ui::RootWindow * root, Widget * target, std::string const & contents) {
            contents_->setText(contents);
            target_ = target;
            // resize to almost nothing so that the show will resize the panel accordingly wrt the actual label size. 
            resize(1, 1);
            Modal<PasteAlert>::show(root, ui::Layout::HorizontalBottom);
        }

        void dismiss() override {
            target_ = nullptr;
            Modal<PasteAlert>::dismiss();
        }

    protected:

        /** TODO this is an ugly hack, should be fixed by allowing the panel and label to be autosized, but I need to figure out a simple API for that. 
         */
        void updateSize(int width, int height) override {
            if (rootWindow() != nullptr) {
                height = ui::Canvas::TextHeight(contents_->text(), ui::Font{}, width) + 1;
                if (height > rootWindow()->height())
                    height = rootWindow()->height();
                contents_->resize(width, height - 1);
            }
            Panel::updateSize(width, height);
        }

        void keyDown(ui::Key k) override {
            if (k == ui::Key::V + ui::Key::Ctrl + ui::Key::Shift) {
                target_->paste(contents_->text());
                dismiss();
            } else if (k == ui::Key::Esc) {
                dismiss();
            }
        }

    private:
        ui::Label * contents_;
        Widget * target_;
    };

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

            pasteAlert_ = new PasteAlert();

            focusWidget(terminal_, true);
            if (! config.session.log().empty()) {
                logFile_.open(config.session.log());
                terminal_->onInput.setHandler(&Session::terminalInputProcessed, this);
            }
        }

        ~Session() override {
            hideModalWidget();
            delete about_;
            delete pasteAlert_;
        }

    protected:

        void attachRenderer(ui::Renderer * renderer) override {
            RootWindow::attachRenderer(renderer);
            // start the terminal once all configuration is done
            terminal_->start();
        }

        void paste(Widget * target, std::string const & contents) override {
            if (contents.empty())
                return;
            if (target != terminal_)
                RootWindow::paste(target, contents);
            std::string confirmPaste = Config::Instance().session.confirmPaste();
            // if we are pasting to the terminal, the warning needs to be displayed
            if (confirmPaste == "always" || (confirmPaste == "multiline" && helpers::NumLines(contents) > 1))
                pasteAlert_->show(this, target, contents);
            else
                RootWindow::paste(target, contents);
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
                if (! isModal() && k == SHORTCUT_ABOUT)
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
        PasteAlert * pasteAlert_;

        std::ofstream logFile_;

        bool closeOnKeyDown_;

        RemoteFiles remoteFiles_;

    }; // tpp::Session

} // namespace tpp

