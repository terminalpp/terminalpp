#pragma once

#include "stamp.h"

#include "ui/widgets/panel.h"
#include "ui/traits/modal.h"

namespace tpp {

    class AboutBox : public ui::Dialog::Cancel {
    public:
        AboutBox():
            Cancel{"Terminal++", /* deleteOnDismiss */ true},
            btnNewIssue_{new Button{" new issue "}},
            btnWWW_{new Button{" www "}} {
            setWidthHint(SizeHint::Manual());
            setHeightHint(SizeHint::Manual());
            setSemanticStyle(SemanticStyle::Primary);
            resize(65,8);
            btnNewIssue_->onMouseClick.setHandler([](UIEvent<MouseButtonEvent>::Payload &) {
                Application::Instance()->createNewIssue("", "Please check that a similar bug has not been already filed. If not, fill in the description and titke of the bug, keeping the version information below. Thank you!");
            });
            btnWWW_->onMouseClick.setHandler([](UIEvent<MouseButtonEvent>::Payload &){
                Application::Instance()->openUrl("https://terminalpp.com");
            });
            addHeaderButton(btnNewIssue_);
            addHeaderButton(btnWWW_);
        }


    protected:

        void paint(Canvas & canvas) override {
            Cancel::paint(canvas);
            canvas.setFg(Color::White);
            //canvas.setFont(ui::Font{}.setSize(2));
            //canvas.textOut(Point{20,1}, "Terminal++");
            //canvas.setFont(ui::Font{});
            if (stamp::version.empty()) {
                canvas.textOut(Point{3,2}, STR("commit:   " << stamp::commit << (stamp::dirty ? "*" : "")));
                canvas.textOut(Point{13,3}, stamp::build_time);
            } else {
                canvas.textOut(Point{3,2}, STR("version:  " << stamp::version));
                canvas.textOut(Point{13,3}, STR(stamp::commit << (stamp::dirty ? "*" : "")));
                canvas.textOut(Point{13,4}, stamp::build_time);
            }
#if (defined RENDERER_QT)
            canvas.textOut(Point{3, 6}, STR("platform: " << ARCH << "(Qt) " << ARCH_SIZE << " " << ARCH_COMPILER << " " << ARCH_COMPILER_VERSION << " " << stamp::build));
#else
            canvas.textOut(Point{3, 6}, STR("platform: " << ARCH << "(native) " << ARCH_SIZE << " " << ARCH_COMPILER << " " << ARCH_COMPILER_VERSION << " " << stamp::build));
#endif
            //canvas.setFont(canvas.font().setBlink(true));
            //canvas.textOut(Point{20, 7}, "Hit a key to dismiss");
            //canvas.setFont(ui::Font{});
        }

        void mouseClick(UIEvent<MouseButtonEvent>::Payload & event) override {
            MARK_AS_UNUSED(event);
            dismiss(this);
        }

        void keyDown(UIEvent<Key>::Payload & event) override {
            MARK_AS_UNUSED(event);
            dismiss(this);
        }

    private:

        Button * btnNewIssue_;
        Button * btnWWW_;

    }; // tpp::AboutBox

} // namespace tpp
