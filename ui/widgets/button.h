#pragma once

#include "../widget.h"

namespace ui {

    class Button : public virtual Widget {
    public:

        Button(std::string const & text):
            text_{text} {
            setHeightHint(SizeHint::AutoSize());
            setWidthHint(SizeHint::AutoSize());
            setFocusable(true);
            setBackground(Color::Yellow);
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
            if (focused())
                canvas.fill(canvas.rect(), Color::Cyan);
            else 
                canvas.textOut(Point{0,0}, text_);
        }

        Size getAutosizeHint() override {
            return Size{static_cast<int>(text_.size()), 1};
        }

    private:
        std::string text_;

    }; // ui::Button

} // namespace ui