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

        Color background_ = Color::Red;
        Border border_;

    }; // ui::CustomPanel


} // namespace ui