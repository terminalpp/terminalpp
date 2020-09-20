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
            setBackground(Color::Blue);
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
            if (focused())
                canvas.setBorder(canvas.rect(), Border::All(Color::Cyan, Border::Kind::Thin));
        }

        Size getAutosizeHint() override {
            return Size{static_cast<int>(text_.size()), 1};
        }

    private:
        std::string text_;

    }; // ui::Button

} // namespace ui