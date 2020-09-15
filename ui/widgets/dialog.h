#pragma once

#include "../mixins/dismissable.h"

#include "panel.h"
#include "button.h"

namespace ui {

    class Dialog : public virtual Widget, public Dismissable {
    public:

        class Cancel;
        class YesNoCancel;

        class Error;

        Dialog(std::string const & title):
            title_{title},
            header_{new Panel{new Layout::Row{HorizontalAlign::Right}}} {
            setLayout(new Layout::Column{});
            header_->setHeightHint(SizeHint::AutoSize());
            header_->setBackground(Color::Red);
            attach(header_);
            setHeightHint(SizeHint::AutoSize());
        }

        std::string const & title() const {
            return title_;
        }

        virtual void setTitle(std::string const & value) {
            if (title_ != value) {
                title_ = value;
                repaint();
            }
        }

        /** Returns the body of the dialog. 
         */
        Widget * body() {
            return body_;
        }

        /** Sets the body widget of the element. 
         */
        virtual void setBody(Widget * value) {
            if (body_ != value) {
                if (body_ != nullptr) {
                    detach(body_);
                    delete body_;
                    body_ = nullptr;
                }
                body_ = value;
                if (body_ != nullptr)
                    attach(body_);
            }
        }

    protected:

        void addHeaderButton(Widget * widget) {
            header_->attachBack(widget);
        }

        /** Dismisses the dialog if its buttons were clicked and the event propageted. 
         
            Since the header buttons do not have registered handlers their click events will propagate to their parent dialog. 
            
            If a button is not to be used for dismissal, its click event should be stopped. 
         */
        void mouseClick(MouseButtonEvent::Payload & e) override {
            if (dynamic_cast<Button*>(e.sender())) {
                dismiss(e.sender());
            } else {
                Widget::mouseClick(e);
            }
        }

        /** The dialog window suppports tab focus. 
         */
        void keyDown(KeyEvent::Payload & e) override {
            if (*e == Key::Tab) {
                renderer()->setKeyboardFocus(renderer()->nextKeyboardFocus());
                e.stop();
            } else if (*e == Key::Tab + Key::Shift) {
                renderer()->setKeyboardFocus(renderer()->prevKeyboardFocus());
                e.stop();
            } 
            Widget::keyDown(e);
        }

    private:

        std::string title_;
        Panel * header_;
        Widget * body_ = nullptr;

    }; // ui::Dialog 

    class Dialog::Cancel : public Dialog {
    public:
        Cancel(std::string const & title):
            Dialog{title},
            btnCancel_{new Button{" X "}} {
            addHeaderButton(btnCancel_);
        }

        Button * btnCancel() const {
            return btnCancel_;
        }

        void cancel() {
            dismiss(btnCancel_);
        }

    protected:

        void keyDown(KeyEvent::Payload & event) override {
            if (*event == Key::Esc) {
                dismiss(btnCancel_);
            } else {
                Dialog::keyDown(event);
            }
        }

    private:

        Button * btnCancel_;

    }; // ui::Dialog::Cancel

    class Dialog::YesNoCancel : public Dialog {
    public:
        YesNoCancel(std::string const & title):
            Dialog{title},
            btnYes_{new Button{" Yes "}},
            btnNo_{new Button{" No "}},
            btnCancel_{new Button{" X "}} {
            addHeaderButton(btnCancel_);
            addHeaderButton(btnNo_);
            addHeaderButton(btnYes_);
        }

        Button * btnCancel() const {
            return btnCancel_;
        }

        Button * btnYes() const {
            return btnYes_;
        }

        Button * btnNo() const {
            return btnNo_;
        }

        void cancel() {
            dismiss(btnCancel_);
        }

    protected:

        void keyDown(KeyEvent::Payload & event) override {
            if (*event == Key::Esc) {
                dismiss(btnCancel_);
            } else {
                Dialog::keyDown(event);
            }
        }

    private:

        Button * btnYes_;
        Button * btnNo_;
        Button * btnCancel_;

    }; // ui::Dialog::YesNoCancel

    /** Error dialog. 
     */
    class Dialog::Error : public Dialog::Cancel {
    public:
        Error(std::string const & message):
            Dialog::Cancel{"Error"},
            contents_{new Label{message}} {
            setBody(contents_);
        }

    private:
        Label * contents_;
    };


} // namespace ui