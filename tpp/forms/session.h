#pragma once 

#include "ui/widgets/label.h"
#include "ui/root_window.h"
#include "ui/builders.h"

#include "vterm/vt100.h"

#include "../config.h"

#include "about_box.h"


namespace tpp {

    /** A window displaying single session. 
      
        The session consists of a terminal attached to a PTY and corresponding ui, such as the information about the program, etc. 
     */
    class Session : public ui::RootWindow {
    public:
        Session(vterm::PTY * pty, vterm::VT100::Palette * palette):
            pty_(pty),
            closeOnKeyDown_(false) {
            Config const & config = Config::Instance();
            using namespace ui;
            using namespace vterm;
            Create(this) 
                /*
                << Layout::Horizontal()
                << (Create(new ui::Label()))
                */
                << Layout::Maximized()
                << (Create(terminal_ = new VT100(config.sessionCols(), config.sessionRows(), palette, pty, config.rendererFps()))
                    << FocusIndex(0)
                    << FocusStop(true)
                    << OnTitleChange(CreateHandler<StringEvent, Session, &Session::terminalTitleChanged>(this))
                    << OnNotification(CreateHandler<VoidEvent, Session, &Session::terminalNotification>(this))
                    << OnPTYTerminated(CreateHandler<ExitCodeEvent, Session, &Session::ptyTerminated>(this))
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

        void terminalNotification(ui::VoidEvent & e) {
            MARK_AS_UNUSED(e);
            setIcon(Icon::Notification);
        }

        void ptyTerminated(vterm::ExitCodeEvent & e) {
            setTitle(STR("Attached process terminated (code " << *e << ") - press a key to exit"));
            setIcon(Icon::Notification);
            // disable the terminal
            terminal_->setEnabled(false);
            // close the window upon next key down
            closeOnKeyDown_ = true;
        }

        void terminalInputProcessed(vterm::InputProcessedEvent & e) {
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

        vterm::PTY * pty_;
        vterm::Terminal * terminal_;
        AboutBox * about_;

        std::ofstream logFile_;

        bool closeOnKeyDown_;

    }; // tpp::Session

} // namespace tpp

