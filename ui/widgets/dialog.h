#pragma once

#include "../mixins/dismissable.h"
#include "../mixins/actionable.h"

#include "panel.h"
#include "label.h"
#include "button.h"

namespace ui {

    class Dialog : public virtual Widget, public Dismissable {
    public:

        class Cancel;
        class YesNoCancel;

        class Error;

        explicit Dialog(std::string const & title):
            title_{new Label(title)},
            header_{new Panel{new Layout::Row{HorizontalAlign::Left}}} {
            setLayout(new Layout::Column{});
            header_->setHeightHint(SizeHint::AutoSize());
            header_->setBackground(Color::Blue);
            attach(header_);
            setHeightHint(SizeHint::AutoSize());
            setBackground(Color::Blue);
            title_->setHeightHint(SizeHint::AutoLayout());
            header_->attach(title_);
        }

        std::string const & title() const {
            return title_->text();
        }

        virtual void setTitle(std::string const & value) {
            title_->setText(value);
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

        /** Adds a button to the header. 
         
            The button can really be any widget, but if the widget is actionable and its executed event is not attached, a dialog dismissal will automatically be attached to it. 

            TODO when proper actions, detecting whether the action is attached or not will change.
         */
        void addHeaderButton(Widget * widget) {
            header_->attachBack(widget);
            header_->attachBack(title_);
            if (Actionable * a = dynamic_cast<Actionable*>(widget)) {
                if (! a->onExecuted.attached())
                    a->onExecuted.setHandler([this](VoidEvent::Payload & e){ dismiss(e.sender()); });
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

        Label * title_ = nullptr;
        Panel * header_ = nullptr;
        Widget * body_ = nullptr;

    }; // ui::Dialog 

    class Dialog::Cancel : public Dialog {
    public:
        explicit Cancel(std::string const & title):
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
        explicit YesNoCancel(std::string const & title):
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

        void keyDown(KeyEvent::Payload & e) override {
            if (*e == Key::Esc) {
                dismiss(btnCancel_);
            } else {
                Dialog::keyDown(e);
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
        explicit Error(std::string const & message):
            Dialog::Cancel{"Error"},
            contents_{new Label{message}} {
            setBody(contents_);
        }

    private:
        Label * contents_;
    };


} // namespace ui