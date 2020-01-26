#pragma once 

#include "widget.h"
#include "root_window.h"

namespace ui {

    /** A simple modal widget.
     */
    class ModalWidget : public Widget {
    public:

        ModalWidget() {
            setWidthHint(ui::Layout::SizeHint::Fixed());
            setHeightHint(ui::Layout::SizeHint::Fixed());
        }

        Event<void> onDismissed;

        virtual void show(RootWindow * root) {
            if (rootWindow() == nullptr)
                root->showModalWidget(this, this);
        }

        void dismiss() {
            ASSERT(rootWindow() != nullptr) << "Modal widget must be shown before it can be dismissed";
            if (rootWindow() != nullptr) {
                onDismissed(this);
                rootWindow()->hideModalWidget();
            }
        }
        
    }; // ui::ModalWidget

} // namespace ui