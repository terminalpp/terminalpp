#pragma once

#include "../mixins/dismissable.h"

#include "panel.h"
#include "button.h"

namespace ui {

    class Dialog : public virtual Widget, public Dismissable {
    public:

        class Cancel;

        Dialog(std::string const & title):
            title_{title},
            header_{new Panel{new Layout::Row{HorizontalAlign::Right}}} {
            setLayout(new Layout::Column{});
            header_->setHeightHint(SizeHint::AutoLayout());
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

        void mouseClick(MouseButtonEvent::Payload & event) override {
            // don't propagate further since the button click is used to dismiss the dialog
            //Widget::mouseClick(event);
            dismiss(event.sender());
            //Event<Widget*>::Payload p{this};
            //onDismiss(p, this);
        }

        /** The dialog window suppports tab focus. 
         */
        void keyDown(KeyEvent::Payload & e) override {
            if (*e == Key::Tab)
                renderer()->setKeyboardFocus(renderer()->nextKeyboardFocus());
            else if (*e == Key::Tab + Key::Shift)
                renderer()->setKeyboardFocus(renderer()->prevKeyboardFocus());
            else
                Widget::keyDown(e);
        }

    private:

        std::string title_;
        Panel * header_;
        Widget * body_;

    }; // ui::Dialog 

    class Dialog::Cancel : public Dialog {
    public:
        Cancel(std::string const & title):
            Dialog{title},
            btnCancel_{new Button{" X "}} {
            addHeaderButton(btnCancel_);
        }

        Button const * btnCancel() const {
            return btnCancel_;
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



    }; 

} // namespace ui