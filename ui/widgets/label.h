#pragma once

#include "../widget.h"


namespace ui {

    class Label : public Widget {
    public:

        Label() = default;

        Label(std::string const & text):
            text_{text} {
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
                repaint();
            }
        }

        Color background() const {
            return background_;
        }

        virtual void setBackground(Color value) {
            if (background_ != value) {
                background_ = value;
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
                relayout();
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
                result.setHeight(static_cast<int>(format_.size()));
            return result;
        }

        void paint(Canvas & canvas) override {
            Point x{0,0};
            canvas.setFg(color_);
            canvas.setBg(background_);
            canvas.setFont(font_);
            canvas.fill(canvas.rect());
            // adjust the text output vertically
            switch (vAlign_) {
                case VerticalAlign::Top:
                    break; // keep the 0
                case VerticalAlign::Middle:
                    x.setY(static_cast<int>((canvas.size().height() - format_.size() * font_.height()) / 2));
                    break;
                case VerticalAlign::Bottom:
                    x.setY(static_cast<int>(canvas.size().height() - format_.size() * font_.height()));
                    break;
            }
            // adjust and print each line
            for (Canvas::TextLine const & line : format_) {
                switch(hAlign_) {
                    case HorizontalAlign::Left:
                        break; // keep the 0
                    case HorizontalAlign::Center:
                        x.setX((canvas.size().width() - line.width) / 2);
                        break;
                    case HorizontalAlign::Right:
                        x.setX(canvas.size().width() - line.width);
                        break;
                }
                canvas.textOut(x, line.begin, line.end);
                x.setY(x.y() + font_.height());
            }
        }


        std::string text_;
        Color color_;
        Color background_;
        Font font_;
        HorizontalAlign hAlign_;
        VerticalAlign vAlign_;
        bool wordWrap_;

        /** Actual line information for fast rendering and positioning, takes word wrap into account. */
        std::vector<Canvas::TextLine> format_;


    };



} // namespace ui
