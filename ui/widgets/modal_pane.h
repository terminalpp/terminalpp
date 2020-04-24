#pragma once

#include "../container.h"

#include "../traits/widget_background.h"

namespace ui {

    /** Modal pane
     
        Starts invisible, then gets items 
     */
    class ModalPane : public Container, public WidgetBackground<ModalPane> {
    public:

        ModalPane():
            WidgetBackground{Color::Black.withAlpha(128)} {
            setVisible(false);
        }

        /** Displays the given widget modally. 
         */
        virtual void show(Widget * widget) {
            ASSERT(renderer() != nullptr);
            add(widget);
            setVisible(true);
            renderer()->setModalRoot(this);
        }

        /** Called when the given modal widget should be dismissed. 
         
            The dismissed widget is detached from the modal pane and if there are no more widgets in it, the modal pane hides itself, returning the focus to the widget that held it before
         */
        virtual void dismiss(Widget * widget) {
            ASSERT(renderer() != nullptr);
            remove(widget);
            if (children_.empty()) {
                setVisible(false);
                renderer()->setModalRoot(renderer()->rootWidget());
            }
        }

        using Container::setLayout;

    protected:

        void paint(Canvas & canvas) {
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

    };


} // namespace ui