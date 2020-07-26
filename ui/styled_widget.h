#pragma once

#include "widget.h"

namespace ui {

    class StyledWidget : public virtual Widget {
    public:
        Color background() const {
            return background_;
        }

        virtual void setBackground(Color value) {
            if (background_ != value) {
                background_ = value;
                repaint();
            }
        }

        void repaint() override {
            if (background_.opaque() || parent() == nullptr)
                Widget::repaint();
            else if (parent() != nullptr) 
                parent()->repaint();
        }

    protected:

        bool allowRepaintRequest(Widget * immediateChild) override {
            if (! border_.empty()) {
                repaint();
                return false;
            }
            return true;
        }

        void paint(Canvas & canvas) override {
            paintBackground(canvas);
            Widget::paint(canvas);
            paintBorder(canvas);
        }

        void paintBackground(Canvas & canvas) {
            canvas.setBg(background_);
            canvas.fill(canvas.rect());
        }

        void paintBorder(Canvas & canvas) {
            // TODO draw the border
        }

    private:

        Color background_;
        Border border_;

    }; // StyledWidget

} // namespace ui