#pragma once

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

    private:

        Color background_;
        Border border_;

    }; // ui::CustomPanel


} // namespace ui