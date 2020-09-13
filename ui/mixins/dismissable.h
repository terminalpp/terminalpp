#pragma once

#include "../widget.h"

namespace ui {

    class Dismissable : public virtual Widget {
    public:

        Event<Widget*> onDismiss;

        virtual void dismiss(Widget * cause) {
            ASSERT(parent() != nullptr);
            // first call the onDismiss event
            Event<Widget*>::Payload p{cause};
            onDismiss(p, this);
            // if the action is still valid, detach from parent
            if (!p.active())
                return;
            parent()->detach(this);
            // and delete itself, if requested
            if (deleteOnDismiss_)
                delete this;
        }

    protected:

        Dismissable(bool deleteOnDismiss = true):
            deleteOnDismiss_{deleteOnDismiss} {
        }

        bool deleteOnDismiss() const {
            return deleteOnDismiss_;
        }

        void setDeleteOnDismiss(bool value = true) {
            deleteOnDismiss_ = value;
        }
        
    private:
        bool deleteOnDismiss_;

    }; // ui::Dismissable

} // namespace ui