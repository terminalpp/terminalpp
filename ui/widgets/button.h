#pragma once

#include "../widget.h"

namespace ui {

    /** A simple clickable and focusable button. 
     */
    class Button : public Widget {
    public:

        std::string const & caption() const {
            return caption_;
        }

        void setCaption(std::string const & value) {
            if (caption_ != value) {
                caption_ = value;
                repaint();
            }
        }

        bool checked() const {
            return checked_;
        }

        void setChecked(bool value) {
            if (checked_ != value)
                updateChecked(value);
        }

    protected:

        virtual void updateChecked(bool value) {
            // TODO trigger the checked events
            repaint();
        }

        void paint(Canvas & canvas) override {

        }

        std::string caption_;
        bool checked_;


    }; // ui::Button


} // namespace ui