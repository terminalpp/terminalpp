#pragma once

#include "../layouts/row.h"
#include "../layouts/column.h"
#include "../layouts/maximize.h"

#include "../traits/modal.h"
#include "../traits/styled.h"


#include "panel.h"
#include "label.h"
#include "button.h"

namespace ui {

    /** A dialog window. 
     
        Dialog window contains header, buttons. 

     */
    class Dialog : public CustomPanel, public Modal<Dialog>, public Styled<Dialog>  {
    public:

        class Cancel;
        class YesNoCancel;

        Dialog(std::string const & title, bool deleteOnDismiss = false):
            Modal{deleteOnDismiss},
            title_{title},
            header_{new PublicContainer{new RowLayout{HorizontalAlign::Right}}},
            headerBackground_{Color::Red},
            body_{nullptr} {
            setBackground(Color::DarkRed);
            setLayout(new ColumnLayout{});
            add(header_);
            setHeightHint(SizeHint::Auto());
            header_->setHeightHint(SizeHint::Auto());
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

        Color headerBackground() const {
            return headerBackground_;
        }

        virtual void setHeaderBackground(Color value) {
            if (headerBackground_ != value) {
                headerBackground_ = value;
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
                    remove(body_);
                    delete body_;
                    body_ = nullptr;
                }
                body_ = value;
                if (body_ != nullptr)
                    add(body_);
            }
        }

    protected:

        void restyle() override {
            setBackground(styleBackground());
            setHeaderBackground(styleHighlightBackground());
        }

        /** Paints the dialog. 
         */
        void paint(Canvas & canvas) override {
            WidgetBackground::paint(canvas);
            WidgetBorder::paint(canvas);
            canvas.setBg(headerBackground_);
            canvas.fillRect(header_->rect());
            canvas.textOut(Point{0,0}, title_);

            Container::paint(canvas);
        }

        void headerButtonClicked(UIEvent<MouseButtonEvent>::Payload & e) {
            dismiss(e.sender());
        }

        void addHeaderButton(Widget * widget) {
            header_->addBack(widget);
        }

        void mouseClick(UIEvent<MouseButtonEvent>::Payload & event) override {
            Container::mouseClick(event);
            UIEvent<Widget*>::Payload p{this};
            onDismiss(p, this);
        }

        std::string title_;
        PublicContainer * header_;
        Color headerBackground_;
        Widget * body_;

    }; // ui::Dialog


    class Dialog::Cancel : public Dialog {
    public:
        Cancel(std::string const & title, bool deleteOnDismiss = false):
            Dialog{title, deleteOnDismiss},
            btnCancel_{new Button{" X "}} {
            btnCancel_->onMouseClick.setHandler(&Dialog::headerButtonClicked, static_cast<Dialog*>(this));
            addHeaderButton(btnCancel_);
        }

        Button const * btnCancel() const {
            return btnCancel_;
        }

        void keyDown(UIEvent<Key>::Payload & event) override {
            if (*event == Key::Esc) {
                dismiss(btnCancel_);
                return;
            }
            Dialog::keyDown(event);
        }

        Button * btnCancel_;

    }; // ui::Dialog::Cancel

    /** A Dialog window template which contains the Yes, No and Cancel buttons. 
     */
    class Dialog::YesNoCancel : public Dialog {
    public:
        YesNoCancel(std::string const & title, bool deleteOnDismiss = false):
            Dialog{title, deleteOnDismiss},
            btnYes_{new Button{" Yes "}},
            btnNo_{new Button{" No "}},
            btnCancel_{new Button{" X "}} {
            btnYes_->onMouseClick.setHandler(&Dialog::headerButtonClicked, static_cast<Dialog*>(this));
            btnNo_->onMouseClick.setHandler(&Dialog::headerButtonClicked, static_cast<Dialog*>(this));
            btnCancel_->onMouseClick.setHandler(&Dialog::headerButtonClicked, static_cast<Dialog*>(this));
            addHeaderButton(btnCancel_);
            addHeaderButton(btnNo_);
            addHeaderButton(btnYes_);
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

        void keyDown(UIEvent<Key>::Payload & event) override {
            if (*event == Key::Esc) {
                dismiss(btnCancel_);
                return;
            }
            Dialog::keyDown(event);
        }

        Button * btnYes_;
        Button * btnNo_;
        Button * btnCancel_;

    }; // ui::Dialog::YesNoCancel 



} // namespace ui