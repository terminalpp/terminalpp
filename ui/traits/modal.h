#pragma once

namespace ui {

    /** Trait for modal widgets. 
     
        Implements the show() method that displays the widget modally on a given root window, the properties which specify the position of the modal widget on the root window and the act of dismissing the widget. 
     */
    template<typename T>
    class Modal : public TraitBase<Modal, T> {
    public:

        Event<void> onDismissed;

        virtual void show(RootWindow * root) {
            if (downcastThis()->rootWindow() == nullptr)
                root->showModalWidget(downcastThis(), downcastThis());
        }

        void dismiss() {
            ASSERT(downcastThis()->rootWindow() != nullptr) << "Modal widget must be shown before it can be dismissed";
            if (downcastThis()->rootWindow() != nullptr) {
                onDismissed(downcastThis());
                downcastThis()->rootWindow()->hideModalWidget();
            }
        }

    protected:
        using TraitBase<Modal, T>::downcastThis;

    }; // ui::Modal

} // namespace ui