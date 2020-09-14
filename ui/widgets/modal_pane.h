#pragma once

#include "../renderer.h"

#include "panel.h"

namespace ui {

    /** Holder for modal widgets. 
     
        A simple panel which is automatically invisible if it contains no children and sets itself visible and as a modal root whenever it has any children. 
     */
    class ModalPane : public Panel {
    public:

        ModalPane(): 
            Panel{new Layout::Column{VerticalAlign::Bottom}} {
            setVisible(false);
            setBackground(Color::Black.withAlpha(128));
        }

        void attach(Widget * widget) override {
            Panel::attach(widget);
            setAsModalRoot();
        }

        void attachBack(Widget * widget) override {
            Panel::attachBack(widget);
            if (children().size() == 1)
                setAsModalRoot();
        }

        void detach(Widget * widget) override {
            Panel::detach(widget);
            if (children().empty() && renderer() != nullptr) {
                ASSERT(renderer()->modalRoot() == this);
                renderer()->setModalRoot(renderer()->root());
                setVisible(false);
            }
        }

    protected:

        void setAsModalRoot() {
            if (renderer() != nullptr && renderer()->modalRoot() != this) {
                ASSERT(renderer()->modalRoot() == renderer()->root()) << "Multiple active modal widgets are not allowed";
                setVisible(true);
                renderer()->setModalRoot(this);
            }
        }

        void keyDown(KeyEvent::Payload & e) override {
            onKeyDown(e, this);
        }

        void keyUp(KeyEvent::Payload & e) override {
            onKeyUp(e, this);
        }

        void keyChar(KeyCharEvent::Payload & e) override {
            onKeyChar(e, this);
        }



    }; // ui::ModalPane


} // namespace ui