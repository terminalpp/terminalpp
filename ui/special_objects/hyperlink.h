#pragma once

#include "helpers/helpers.h"

#include "../canvas.h"

namespace ui {

    /** Hyperlink information.
     
        Contains the minimal necessary information for a hyperlink such as its url and style. Also keeps track of whether the hyperlink is currently active (mouse over) and the associated style. 
     */
    class Hyperlink : public Canvas::SpecialObject {
    public:

        /** Style for the hyperlink. 
         
            The foreground and background colors are blended over existing colors of the cell. Only font attributes are used from the font itself. 
         */
        struct Style {
            Color fg;
            Color bg;
            Font font;

            Style():
                fg{Color::None},
                bg{Color::None} {
            }

            Style(Color fg, Color bg, Font font):
                fg{fg},
                bg{bg},
                font{font} {
            }

            /** Applies the style to given cell. 
             */
            void applyTo(Canvas::Cell & cell) {
                switch (fg.a) {
                    case Color::OPAQUE:
                        cell.setFg(fg);
                        break;
                    case Color::FULLY_TRANSPARENT:
                        break;
                    default:
                        cell.setFg(fg.blendOver(cell.fg()));
                        break;
                }
                switch (bg.a) {
                    case Color::OPAQUE:
                        cell.setBg(bg);
                        break;
                    case Color::FULLY_TRANSPARENT:
                        break;
                    default:
                        cell.setBg(bg.blendOver(cell.fg()));
                        break;
                }
                cell.font().orAttributesFrom(font);
            }
        };

        /** Shorthand for hyperlink special object smart pointer. 
         */
        using Ptr = Canvas::SpecialObject::Ptr<Hyperlink>;

        /** Creates a hyperlink to given url. 
         */
        Hyperlink(std::string const & url):
            url_{url} {
        } 

        /** Creates a hyperlink to given url and specifies its normal and active styles. 
         */
        Hyperlink(std::string const & url, Style const & normal, Style const & active):
            url_{url},
            normalStyle_{normal},
            activeStyle_{active} {
        } 

        std::string const & url() const {
            return url_;
        }

        void setUrl(std::string const & url) {
            url_ = url;
        }

        bool active() const {
            return active_;
        }

        void setActive(bool value = true) {
            active_ = value;
        }

        /** \name Hyperlink style
         */
        //@{
        Style const & normalStyle() const {
            return normalStyle_;
        }

        void setNormalStyle(Style const & value) {
            normalStyle_ = value;
        }
        //@}

        /** \name Hyperlink active style. 
         
            Used when the link is active, i.e. mouse over. 
         */
        //@{
        Style const & activeStyle() const {
            return activeStyle_;
        }

        void setActiveStyle(Style const & value) {
            activeStyle_ = value;
        }
        //@}

    protected:

        /** Applies the normal or active style to the fallback cells. 
         */
        void updateFallbackCell(Canvas::Cell & fallback, Canvas::Cell const & original) override {
            MARK_AS_UNUSED(original);
            if (active())
                activeStyle_.applyTo(fallback);
            else  
                normalStyle_.applyTo(fallback);
        }

    private:
        std::string url_;

        bool active_ = false;

        Style normalStyle_;
        Style activeStyle_;

    }; // ui::Hyperlink

}