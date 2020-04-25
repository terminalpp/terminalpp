#pragma once

#include "../widget.h"

namespace ui {

    /** Text label. 
     */
    class Label : public PublicWidget {
    public:

        Label(std::string const & text):
            text_{text} {
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
                repaint();
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
                repaint();
            }
        }
        //@}

        /** \name Auto size
         */
        //@{
        bool autoSize() const {
            return autoSize_;
        }

        void setAutoSize(bool value) {
            if (autoSize_ != value) {
                autoSize_ = value;
                // TODO update the size 
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

        void paint(Canvas & canvas) override {
            canvas.textOut(Point{0,0}, text_);
        }

        std::pair<int, int> calculateAutoSize() override {
            return std::make_pair(static_cast<int>(text_.size()), 1);
        }

    private:
        
        std::string text_;
        bool wordWrap_;
        bool autoSize_;
        HorizontalAlign hAlign_;
        VerticalAlign vAlign_;

    }; // ui::Label

} // namespace ui