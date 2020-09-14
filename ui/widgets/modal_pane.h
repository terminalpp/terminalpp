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

        /** \name Input events. 
         
            All input events that would normally propagate to parents are stopped by the modal pane so that they won't get higher. 
         */
        //@{
        void keyDown(KeyEvent::Payload & e) override {
            e.stop();
            Widget::keyDown(e);
        }

        void keyUp(KeyEvent::Payload & e) override {
            e.stop();
            Widget::keyUp(e);
        }

        void keyChar(KeyCharEvent::Payload & e) override {
            e.stop();
            Widget::keyChar(e);
        }

        void mouseMove(MouseMoveEvent::Payload & e) override {
            e.stop();
            Widget::mouseMove(e);
        }

        void mouseWheel(MouseWheelEvent::Payload & e) override {
            e.stop();
            Widget::mouseWheel(e);
        }

        void mouseDown(MouseButtonEvent::Payload & e) override {
            e.stop();
            Widget::mouseDown(e);
        }

        void mouseUp(MouseButtonEvent::Payload & e) override {
            e.stop();
            Widget::mouseUp(e);
        }

        void mouseClick(MouseButtonEvent::Payload & e) override {
            e.stop();
            Widget::mouseClick(e);
        }

        void mouseDoubleClick(MouseButtonEvent::Payload & e) override {
            e.stop();
            Widget::mouseDoubleClick(e);
        }
        //@}



    }; // ui::ModalPane


} // namespace ui