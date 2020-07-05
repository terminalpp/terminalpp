#pragma once

#include "../widget.h"

namespace ui3 {

    /** Panel implementation. 
     */
    class CustomPanel : public Widget {
    public:
        void repaint() {
            if (background_.opaque() || parent() == nullptr)
                Widget::repaint();
            else 
                parent()->repaint();
        }

    protected:

        Color background() const {
            return background_;
        }

        virtual void setBackground(Color value) {
            if (background_ != value) {
                background_ = value;
                repaint();
            }
        }

        bool allowRepaintRequest(Widget * immediateChild) override {
            if (! border_.empty()) {
                repaint();
                return false;
            }
            return Widget::allowRepaintRequest(immediateChild);
        }

        void paint(Canvas & canvas) override {
            canvas.fill(canvas.rect(), background_);
            Widget::paint(canvas);
            // TODO draw the border
        }

        Color background_;
        Border border_;

    }; // ui::CustomPanel

    class Panel : public CustomPanel {
    public:
        using Widget::attach;
        using Widget::attachBack;
        using Widget::detach;
        using CustomPanel::background;
        using CustomPanel::setBackground;

    }; // ui::Panel


} // namespace ui