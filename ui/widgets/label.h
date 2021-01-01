#pragma once

#include "../widget.h"


namespace ui {

    class Label : public Widget {
    public:

        Label() = default;

        explicit Label(std::string const & text):
            text_{text} {
            setHeightHint(new SizeHint::AutoSize());
        }

        using Widget::setWidthHint;
        using Widget::setHeightHint;

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
                requestRepaint();
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
                requestRepaint();
            }
        }

        VerticalAlign vAlign() const {
            return vAlign_;
        }

        virtual void setVAlign(VerticalAlign value) {
            if (vAlign_ != value) {
                vAlign_ = value;
                requestRepaint();
            }
        }

        bool wordWrap() const {
            return wordWrap_;
        }

        virtual void setWordWrap(bool value) {
            if (wordWrap_ != value) {
                wordWrap_ = value;
                relayout();
            }
        }

    protected:

        /** Recalculates the cached format of the stored text and then relayouts.
         */
        bool relayout() override {
            if (widthHint()->isAuto()) 
                format_ = Canvas::GetTextMetrics(text_, Canvas::NoWordWrap);
            else
                format_ = Canvas::GetTextMetrics(text_, wordWrap_ ? std::max(1, rect().width()) : Canvas::NoWordWrap);
            return Widget::relayout();
        }

    private:

        int getAutoHeight() const override {
            return static_cast<int>(format_.size());
        }

        int getAutoWidth() const override {
            return std::max_element(format_.begin(), format_.end(), [](Canvas::TextLine const & a, Canvas::TextLine const & b){ return a.width < b.width; })->width;
        }
    
        void paint(Canvas & canvas) override {
            Point x{0,0};
            canvas.setFg(color_);
            canvas.setFont(font_);
            // adjust the text output vertically
            switch (vAlign_) {
                case VerticalAlign::Top:
                    break; // keep the 0
                case VerticalAlign::Middle:
                    x.setY(static_cast<int>((canvas.height() - format_.size() * font_.height()) / 2));
                    break;
                case VerticalAlign::Bottom:
                    x.setY(static_cast<int>(canvas.height() - format_.size() * font_.height()));
                    break;
            }
            // adjust and print each line
            for (Canvas::TextLine const & line : format_) {
                switch(hAlign_) {
                    case HorizontalAlign::Left:
                        break; // keep the 0
                    case HorizontalAlign::Center:
                        x.setX((canvas.width() - line.width) / 2);
                        break;
                    case HorizontalAlign::Right:
                        x.setX(canvas.width() - line.width);
                        break;
                }
                canvas.textOut(x, line.begin, line.end);
                x.setY(x.y() + font_.height());
            }
        }


        std::string text_;
        Color color_ = Color::White;
        Font font_;
        HorizontalAlign hAlign_;
        VerticalAlign vAlign_;
        bool wordWrap_;

        /** Actual line information for fast rendering and positioning, takes word wrap into account. */
        std::vector<Canvas::TextLine> format_;


    };



} // namespace ui
