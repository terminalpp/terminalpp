#pragma once

#include "../widget.h"
#include "../mixins/actionable.h"

namespace ui {

    class Button : public virtual Widget, public Actionable {
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

        void mouseClick(MouseButtonEvent::Payload & e) override {
            if (e->button == MouseButton::Left) {
                e.stop();
                Widget::mouseClick(e);
                execute();
            } else {
                Widget::mouseClick(e);
            }
        }

        void keyDown(KeyEvent::Payload & e) override {
            if (*e == Key::Enter) {
                e.stop();
                Widget::keyDown(e);
                execute();
            } else {
                Widget::keyDown(e);
            }
        }

        Size getAutosizeHint() override {
            return Size{static_cast<int>(text_.size()), 1};
        }

    private:
        std::string text_;

    }; // ui::Button

} // namespace ui