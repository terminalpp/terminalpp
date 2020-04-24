#pragma once

#include "panel.h"

namespace ui {

    /** A dialog window. 
     
        Dialog window contains header, buttons. 
     */
    class Dialog : public Panel {
    public:


        Dialog() {
            setBackground(Color::DarkRed);
        }



        /** Triggered when the dialog is to be dismissed. 
         
            The payload of the event is the control responsible for the dismisal which can be used to identify the outcome. 
         */
        Event<Widget*> onDismiss;

    protected:

        virtual void dismiss(Widget * cause) {

        }
        
        void mouseClick(Event<MouseButtonEvent>::Payload & event) override {
            Panel::mouseClick(event);
            Event<Widget*>::Payload p{this};
            onDismiss(p, this);
        }

    }; // ui::Dialog

} // namespace ui