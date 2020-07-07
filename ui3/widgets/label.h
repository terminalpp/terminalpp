#pragma once

#include "../widget.h"


namespace ui3 {

    class Label : public Widget {
    public:
        
        std::string const & text() const {
            return text_;
        }

        virtual void setText(std::string const & value) {
            if (text_ != value) {
                text_ = value;
                relayout();
            }
        }

        Color color() const {
            return color_;
        }

        virtual void setColor(Color value) {
            if (color_ != value) {
                color_ = value;
                repaint();
            }
        }

        Font font() const {
            return font_;
        }

        virtual void setFont(Font value) {
            if (font_ != value) {
                font_ = value;
                relayout();
            }

        }

        HorizontalAlign hAlign() const {
            return hAlign_;
        }

        virtual void setHAlign(HorizontalAlign value) {
            if (hAlign_ != value) {
                hAlign_ = value;
                repaint();
            }
        }

        VerticalAlign vAlign() const {
            return vAlign_;
        }

        virtual void setVAlign(VerticalAlign value) {
            if (vAlign_ != value) {
                vAlign_ = value;
                repaint();
            }
        }

        bool wordWrap() const {
            return wordWrap_;
        }

        virtual void setWordWrap(bool value) {
            if (wordWrap_ != value) {
                wordWrap_ = value;
                repaint();
            }
        }

    private:

        Size getAutosizeHint() override {
            if (widthHint() == SizeHint::AutoSize()) 
                format_ = Canvas::GetTextMetrics(text_, Canvas::NoWordWrap);
            else
                format_ = Canvas::GetTextMetrics(text_, wordWrap_ ? rect().width() : Canvas::NoWordWrap);
            Size result = rect().size();
            if (widthHint() == SizeHint::AutoSize()) {
                result.setWidth(format_[0].width);
                for (size_t i = 1, e = format_.size(); i != e; ++i)
                    if (format_[i].width > result.width())
                        result.setWidth(format_[i].width);
            }            
            if (heightHint() == SizeHint::AutoSize()) 
                result.setHeight(format_.size());
        }

        void paint(Canvas & canvas) override {
            // center & print each line here
        }


        std::string text_;
        Color color_;
        Font font_;
        HorizontalAlign hAlign_;
        VerticalAlign vAlign_;
        bool wordWrap_;

        /** Actual line information for fast rendering and positioning, takes word wrap into account. */
        std::vector<Canvas::TextLine> format_;


    };



} // namespace ui
