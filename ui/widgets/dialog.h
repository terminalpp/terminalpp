#pragma once

#include "../layouts/row.h"
#include "../layouts/column.h"
#include "../layouts/maximize.h"

#include "panel.h"
#include "label.h"

namespace ui {

    /** A dialog window. 
     
        Dialog window contains header, buttons. 

     */
    class Dialog : public CustomPanel  {
    public:

        class YesNoCancel;

        Dialog(std::string const & title):
            title_{title},
            header_{new PublicContainer{new RowLayout{HorizontalAlign::Right}}},
            body_{nullptr} {
            setBackground(Color::DarkRed);
            setLayout(new ColumnLayout{});
            add(header_);
            setHeightHint(SizeHint::Auto());
            header_->setHeightHint(SizeHint::Auto());
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
                    remove(body_);
                    delete body_;
                    body_ = nullptr;
                }
                body_ = value;
                if (body_ != nullptr)
                    add(body_);
            }
        }

        /** Triggered when the dialog is to be dismissed. 
         
            The payload of the event is the control responsible for the dismisal which can be used to identify the outcome. 
         */
        Event<Widget*> onDismiss;


    protected:

        bool delegatePaintToParent() override {
            return WidgetBackground::delegatePaintToParent() || Container::delegatePaintToParent();
        }

        /** Paints the dialog. 
         */
        void paint(Canvas & canvas) override {
            WidgetBackground::paint(canvas);
            WidgetBorder::paint(canvas);
            canvas.setBg(Color::Red);
            canvas.fillRect(header_->rect());
            canvas.textOut(Point{0,0}, title_);

            Container::paint(canvas);
        }

        void addHeaderButton(Widget * widget) {
            header_->add(widget);
        }

        virtual void dismiss(Widget * cause) {
            Event<Widget*>::Payload p{cause};
            onDismiss(p, this);
        }
        
        void mouseClick(Event<MouseButtonEvent>::Payload & event) override {
            Container::mouseClick(event);
            Event<Widget*>::Payload p{this};
            onDismiss(p, this);
        }

        std::string title_;
        PublicContainer * header_;
        Widget * body_;

    }; // ui::Dialog

    /** A Dialog window template which contains the Yes, No and Cancel buttons. 
     */
    class Dialog::YesNoCancel : public Dialog {
    public:
        YesNoCancel(std::string const & title):
            Dialog{title},
            btnYes_{new Button{" Yes "}},
            btnNo_{new Button{" No "}},
            btnCancel_{new Button{" X "}} {
            btnYes_->onMouseClick.setHandler(&YesNoCancel::headerButtonClicked, this);
            btnNo_->onMouseClick.setHandler(&YesNoCancel::headerButtonClicked, this);
            btnCancel_->onMouseClick.setHandler(&YesNoCancel::headerButtonClicked, this);
            addHeaderButton(btnYes_);
            addHeaderButton(btnNo_);
            addHeaderButton(btnCancel_);
        }

        Button const * btnYes() const {
            return btnYes_;
        }

        Button const * btnNo() const {
            return btnNo_;
        }

        Button const * btnCancel() const {
            return btnCancel_;
        }

    protected:
        Button * btnYes_;
        Button * btnNo_;
        Button * btnCancel_;

    private:
        void headerButtonClicked(Event<MouseButtonEvent>::Payload & e) {
            dismiss(e.sender());
        }
    }; // ui::Dialog::YesNoCancel 

} // namespace ui