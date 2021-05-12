#pragma once

#include "stamp.h"

#include "ui/widgets/panel.h"

namespace tpp {

    class AboutBox : public ui::Dialog::Cancel {
    public:
        AboutBox():
            Cancel{"Terminal++"},
            btnNewIssue_{new Button{" new issue "}},
            btnWWW_{new Button{" www "}} {
            setWidthHint(new SizeHint::Manual{});
            setHeightHint(new SizeHint::Manual{});
            //setSemanticStyle(SemanticStyle::Primary);
            resize(Size{65,8});
            btnNewIssue_->onExecuted.setHandler([](VoidEvent::Payload & e) {
                Application::Instance()->createNewIssue("", "Please check that a similar bug has not been already filed. If not, fill in the description and titke of the bug, keeping the version information below. Thank you!");
                e.stop();
            });
            btnWWW_->onExecuted.setHandler([](VoidEvent::Payload & e){
                Application::Instance()->openUrl("https://terminalpp.com");
                e.stop();
            });
            addHeaderButton(btnNewIssue_);
            addHeaderButton(btnWWW_);
        }

    protected:

        void paint(Canvas & canvas) override {
            Cancel::paint(canvas);
            canvas.setFg(Color::White);
            canvas.setFont(ui::Font{}.setSize(2));
            canvas.textOut(Point{20,1}, "Terminal++");
            canvas.setFont(ui::Font{});
            if (stamp::version.empty()) {
                canvas.textOut(Point{3,2}, STR("commit:   " << stamp::commit << (stamp::dirty ? "*" : "")));
                //canvas.textOut(Point{13,3}, stamp::build_time);
            } else {
                canvas.textOut(Point{3,2}, STR("version:  " << stamp::version));
                canvas.textOut(Point{13,3}, STR(stamp::commit << (stamp::dirty ? "*" : "")));
                //canvas.textOut(Point{13,4}, stamp::build_time);
            }
#if (defined RENDERER_QT)
            canvas.textOut(Point{3, 6}, STR("platform: " << ARCH << "(Qt) " << ARCH_SIZE << " " << ARCH_COMPILER << " " << ARCH_COMPILER_VERSION << " " << stamp::build));
#else
            canvas.textOut(Point{3, 6}, STR("platform: " << ARCH << "(native) " << ARCH_SIZE << " " << ARCH_COMPILER << " " << ARCH_COMPILER_VERSION << " " << stamp::build));
#endif
            canvas.font().setBlink(true);
            canvas.textOut(Point{20, 8}, "Hit esc to dismiss");
            canvas.setFont(ui::Font{});
        }

        void mouseClick(MouseButtonEvent::Payload & event) override {
            MARK_AS_UNUSED(event);
            dismiss(this);
        }

    private:

        Button * btnNewIssue_;
        Button * btnWWW_;

    }; // tpp::AboutBox

} // namespace tpp
