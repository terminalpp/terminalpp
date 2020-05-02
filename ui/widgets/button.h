#pragma once

#include "../widget.h"

namespace ui {

    /** A simple button.
     */
    class Button : public PublicWidget {
    public:

        Button(std::string const & text):
            text_{text} {
            setHeightHint(SizeHint::Auto());
            setWidthHint(SizeHint::Auto());
            setFocusable(true);
        }

        /** \name Button text. 
         */
        //@{
        std::string const & text() const {
            return text_;
        }

        virtual void setText(std::string const & value) {
            if (text_ != value) {
                text_ = value;
                repaint();
            }
        }
        //@}

    protected:

        void paint(Canvas & canvas) override {
            canvas.textOut(Point{0,0}, text_);
        }

        Size calculateAutoSize() override {
            return Size{static_cast<int>(text_.size()), 1};
        }

    private:
        std::string text_;
        

    }; // ui::Button

} // namespace ui