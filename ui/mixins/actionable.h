#pragma once

#include "../widget.h"

namespace ui {

    /** Base class for all widgets that can be attached to actions. 
     
        TODO in the future, this is what the action would hook to and work with. But we have no actions yet so this is just used as a mock in button. 
     */
    class Actionable : public virtual Widget {
    public:
        /** Triggered when the widget's action has been executed.
         
            Such as a button was clicked, or was focused while enter was pressed. 
         */
        VoidEvent onExecuted;

        /** Called to execute the action. 
         
            NOTE this has to be the last call, otherwise the automatically deletable dialogs won't work. 
         */
        virtual void execute() {
            ASSERT(enabled()); 
            VoidEvent::Payload p;
            onExecuted(p, this);
        }
    }; // ui::Dismissable

} // namespace ui