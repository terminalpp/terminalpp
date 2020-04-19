#pragma once

#include "panel.h"

namespace ui {

    /** A dialog window. 
     
        Dialog window contains header, message and buttons. 
     */
    class Dialog : public Panel {
    public:

        Dialog() {
            setBackground(Color::DarkRed);
        }

        Event<void> onSuccess;
        Event<void> onCancel;

    }; // ui::Dialog

} // namespace ui