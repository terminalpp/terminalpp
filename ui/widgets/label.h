#pragma once

#include "helpers/string.h"

#include "../widget.h"

namespace ui {

    /** Text label. 
     */
    class Label : public PublicWidget {
    public:

        Label(std::string const & text):
            text_{text},
            wordWrap_{true},
            hAlign_{HorizontalAlign::Left},
            vAlign_{VerticalAlign::Middle} {
            setHeightHint(SizeHint::Auto());
        }

        /** \name Label text. 
         */
        //@{
        std::string const & text() const {
            return text_;
        }

        virtual void setText(std::string const & value) {
            if (text_ != value) {
                text_ = value;
                relayout();
            }
        }
        //@}

        /** \name Word-wrap
         */
        //@{
        bool wordWrap() const {
            return wordWrap_;
        }

        virtual void setWordWrap(bool value = true) {
            if (wordWrap_ != value) {
                wordWrap_ = value;
                relayout();
            }
        }
        //@}

        /** \name Horizontal Alignment
         */
        //@{
        HorizontalAlign horizontalAlign() const {
            return hAlign_;
        }

        virtual void setHorizontalAlign(HorizontalAlign value) {
            if (hAlign_ != value) {
                hAlign_ = value;
                repaint();
            }
        }
        //@}

        /** \name Vertical Alignment
         */
        //@{
        VerticalAlign verticalAlign() const {
            return vAlign_;
        }

        virtual void setVerticalAlign(VerticalAlign value) {
            if (vAlign_ != value) {
                vAlign_ = value;
                repaint();
            }
        }
        //@}

    protected:

        // TODO update maxWidth by font size when label has its font selected and the line width for each line as well
        void updateTextMetrics(int maxWidth) {
            textLines_.clear();
            maxLineWidth_ = 0;
            int wrapAt = wordWrap_ ? maxWidth : helpers::NoWordWrap;
            Char::iterator_utf8 start = Char::BeginOf(text_);
            Char::iterator_utf8 end = Char::EndOf(text_);
            while (start != end) {
                StringLine line = helpers::GetLine(start, end, wrapAt);
                start = line.end;
                if (Char::IsWhitespace(*line.end))
                    ++start;
                if (maxLineWidth_ < line.width)
                    maxLineWidth_ = line.width;
                textLines_.push_back(line);
            }
        }

        void paint(Canvas & canvas) override {
            int row;
            switch(vAlign_) {
                case VerticalAlign::Top:
                default:
                    row = 0;
                    break;
                case VerticalAlign::Middle:
                    row = (canvas.height() - static_cast<int>(textLines_.size())) / 2;
                    break;
                case VerticalAlign::Bottom:
                    row = canvas.height() - static_cast<int>(textLines_.size());
                    break;
            }
            for (helpers::StringLine const & l : textLines_) {
                canvas.textOut(Point{0, row}, l, canvas.width(), hAlign_);
                ++row;
            }
        }

        Size calculateAutoSize() override {
            if (widthHint() != SizeHint::Auto())
                updateTextMetrics(width());
            else
                updateTextMetrics(helpers::NoWordWrap);
            return Size{maxLineWidth_, static_cast<int>(textLines_.size())};
        }

    private:

        std::string text_;
        bool wordWrap_;
        HorizontalAlign hAlign_;
        VerticalAlign vAlign_;

        std::vector<StringLine> textLines_;
        int maxLineWidth_;

    }; // ui::Label

} // namespace ui