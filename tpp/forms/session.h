#include "stamp.h"
#include "helpers/stamp.h"

#include "ui/widgets/label.h"
#include "ui/root_window.h"
#include "ui/builders.h"

#include "vterm/vt100.h"

#include "config.h"


namespace tpp {

    /** A window displaying single session. 
      
        The session consists of a terminal attached to a PTY and corresponding ui, such as the information about the program, etc. 
     */
    class Session : public ui::RootWindow {
    public:
        Session(vterm::PTY * pty, vterm::VT100::Palette * palette):
            pty_(pty) {
            Config const & config = Config::Instance();
            using namespace ui;
            using namespace vterm;
            Create(this) 
                << Layout::Horizontal()
                << (Create(header_ = new Label())
                    << HeightHint(SizeHint::Fixed())
                    << Geometry(1, 2)
                    << ui::Font().setDoubleWidth(false).setSize(2)
                    << STR("\xef\xa1\x96t++ :" << helpers::Stamp::Stored())
                    << OnMouseClick(CreateHandler<MouseButtonEvent, Session, &Session::headerClicked>(this))
                )
                << (Create(terminal_ = new VT100(config.sessionCols(), config.sessionRows(), palette, pty, config.rendererFps()))
                    << OnTitleChange(CreateHandler<StringEvent, Session, &Session::terminalTitleChanged>(this))
                    << OnNotification(CreateHandler<VoidEvent, Session, &Session::terminalNotification>(this))
                    << OnPTYTerminated(CreateHandler<ExitCodeEvent, Session, &Session::ptyTerminated>(this))
                    //<< OnLineScrolledOut(CreateHandler<LineScrollEvent, Session, &Session::lineScrolledOut>(this))
                );
            focusWidget(terminal_, true);
            headerTimer_.onTimer += CreateHandler<helpers::TimerEvent, Session, &Session::headerAutohide>(this);
            //headerTimer_.start(5000, false);
        }


    private:

        void headerClicked(ui::MouseButtonEvent & e) {
            MARK_AS_UNUSED(e);
            header_->setVisible(false);
        }

        void headerAutohide(helpers::TimerEvent & e) {
            MARK_AS_UNUSED(e);
            header_->setVisible(false);
        }

        void terminalTitleChanged(ui::StringEvent & e) {
            setTitle(*e);
        }

        void terminalNotification(ui::VoidEvent & e) {
            MARK_AS_UNUSED(e);
            setIcon(Icon::Notification);
        }

        void ptyTerminated(vterm::ExitCodeEvent & e) {
            header_->setText(STR("Attached process terminated (code " << *e << ")"));
            header_->setVisible(true);
            setIcon(Icon::Notification);
        }

        void lineScrolledOut(vterm::LineScrollEvent & e) {
            LOG << "Scrolled " << *e << " lines";
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
            setIcon(Icon::Default);
            ui::RootWindow::keyDown(k);
        }

        vterm::PTY * pty_;
        vterm::Terminal * terminal_;
        ui::Label * header_;

        helpers::Timer headerTimer_;

    }; // tpp::Session

} // namespace tpp

