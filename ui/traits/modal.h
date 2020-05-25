#pragma once

#include "../widget.h"
#include "../container.h"

#include "trait_base.h"
#include "widget_background.h"


namespace ui {

    /** A container capable of displaying modal widgets. 
     
        A modal pane is invisible if empty, but becomes visible and acquires modal input as soon as it has children. When last child is removed, the modal pane disappears and releases the modal focus. Any widget can be child of a modal pane.
     */
    class ModalPane : public Container, public WidgetBackground<ModalPane> {
    public:

        ModalPane():
            WidgetBackground{Color::Black.withAlpha(128)},
            modal_{true} {
            setVisible(false);
            setLayout(new ColumnLayout{VerticalAlign::Bottom});
        }

        using Container::setLayout;

        using Widget::setWidthHint;
        using Widget::setHeightHint;

        void add(Widget * child) override {
            ASSERT(renderer() != nullptr);
            Container::add(child);
            if (!visible()) {
                setVisible(true);
                if (modal_)
                    renderer()->setModalRoot(this);
            }
        }

        void remove(Widget * child) override {
            Container::remove(child);
            if (children_.empty()) {
                setVisible(false);
                if (renderer()->modalRoot() == this)
                    renderer()->setModalRoot(renderer()->rootWidget());
            } 
        }

        bool modal() {
            return modal_;
        }

        virtual void setModal(bool value = true) {
            if (modal_ != value) {
                modal_ = value;
                if (renderer() != nullptr) {
                    if (modal_ && renderer()->modalRoot() != this)
                        renderer()->setModalRoot(this);
                    else if (!modal_ && renderer()->modalRoot() == this)
                        renderer()->setModalRoot(renderer()->rootWidget());
                }
            }
        }

    protected:

        bool isTransparent() override {
            return WidgetBackground::isTransparent() || Container::isTransparent();
        }

        void paint(Canvas & canvas) override {
            WidgetBackground::paint(canvas);
            Container::paint(canvas);
        }

        Widget * getNextFocusableWidget(Widget * current) override {
            if (enabled()) {
                if (current == nullptr && focusable())
                    return this;
                ASSERT(current == this || current == nullptr || current->parent() == this);
                if (current == this)
                    current = nullptr;
                // unlike container, the modal widget never goes to its parent for next focusable element
                return getNextFocusableChild(current);
            } else {
                return nullptr;
            }
        }

    private:
        bool modal_;

    }; // ui::ModalPane

    /** Modal widgets trait.
        
     */
    template<typename T> 
    class Modal : public TraitBase<Modal, T> {
    public:

        virtual ~Modal() {
        }

        /** Triggered when the dialog is to be dismissed. 
         
            The payload of the event is the control responsible for the dismisal which can be used to identify the outcome. 
         */
        Event<Widget*> onDismiss;

    protected:

        using TraitBase<Modal, T>::downcastThis;

        Modal(bool deleteOnDismiss = false):
            deleteOnDismiss_{deleteOnDismiss} {
        }

        /** Dismisses the widget. 
         
            calls the onDismiss event 
         */
        virtual void dismiss(Widget * cause) {
            // remove from parent
            Container * parent = dynamic_cast<Container*>(downcastThis()->parent());
            ASSERT(parent != nullptr);
            if (parent != nullptr)
                parent->remove(downcastThis());
            // and call the dismiss handler so that it is the very last thing we do and can delete the widget if necessary
            Event<Widget*>::Payload p{cause};
            onDismiss(p, downcastThis());
            // delete the widget if applicable
            if (deleteOnDismiss_)
                delete this;
        }

    private:
        bool deleteOnDismiss_;

    }; // ui::Modal



} // namespace ui