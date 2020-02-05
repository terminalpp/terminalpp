#pragma once

#include <cstdint>

#include "helpers/helpers.h"

#include "shapes.h"
#include "color.h"
#include "font.h"

namespace ui {

    /** Information about cursor, its position and apperance. 
     */
    class Cursor {
    public:
	    Point pos;
        bool visible;
		// focused codepoint, color and blinking
        char32_t activeCodepoint;
        Color activeColor;
        bool activeBlink;
		// non - focused codepoint, color and blinking
		char32_t inactiveCodepoint;
		Color inactiveColor;
		bool inactiveBlink;


        Cursor():
		    pos{0,0},
			visible{true},
			activeCodepoint{0x2588}, // full box
			activeColor{Color::White},
			activeBlink{true},
			inactiveCodepoint{0x2591}, // light shade
			inactiveColor{Color::White},
			inactiveBlink{false} {
        }

		/** Creates a new cursor with position updated by the given point. 
		 */
		Cursor operator + (Point const & p) const {
			Cursor result(*this);
			result.pos += p;
			return result;
		}

        /** Shorthand for creating an invisible cursor description. 
         */
        static Cursor Invisible() {
            Cursor result;
            result.visible = false;
            return result;
        }

    }; // ui::Cursor

    /** Attributes of a cell. 
     */
    class Attributes {
    public:

        Attributes():
            raw_{0} {
        }

        bool empty() const { return raw_ == 0; }
        bool emptyDecorations() const { return (raw_ & (UNDERLINE | STRIKETHROUGH | CURLY_UNDERLINE)) == 0; }
        bool underline() const { return raw_ & UNDERLINE; }
        bool strikethrough() const { return raw_ & STRIKETHROUGH; }
        bool curlyUnderline() const { return raw_ & CURLY_UNDERLINE; }
        bool border() const { return raw_ & (BORDER_LEFT | BORDER_TOP | BORDER_RIGHT | BORDER_BOTTOM); }
        bool borderLeft() const { return raw_ & BORDER_LEFT; }
        bool borderTop() const { return raw_ & BORDER_TOP; }
        bool borderRight() const { return raw_ & BORDER_RIGHT; }
        bool borderBottom() const {	return raw_ & BORDER_BOTTOM; }
        bool borderThick() const { return raw_ & BORDER_THICK; }
        bool blink() const { return raw_ & BLINK; }

        bool operator == (Attributes other) const {
            return raw_ == other.raw_;
        }

        bool operator != (Attributes other) const {
            return raw_ != other.raw_;
        }

        Attributes & setUnderline(bool value = true) { 
            if (value) raw_ |= UNDERLINE; else raw_ &= ~UNDERLINE;
            return *this;
        }
        Attributes & setStrikethrough(bool value = true) { 
            if (value) raw_ |= STRIKETHROUGH; else raw_ &= ~STRIKETHROUGH; 
            return *this;
        }
        Attributes & setCurlyUnderline(bool value = true) { 
            if (value) raw_ |= CURLY_UNDERLINE; else raw_ &= ~CURLY_UNDERLINE; 
            return *this;
        }
        Attributes & setBorderLeft(bool value = true) { 
            if (value) raw_ |= BORDER_LEFT; else raw_ &= ~BORDER_LEFT; 
            return *this;
        }
        Attributes & setBorderTop(bool value = true) { 
            if (value) raw_ |= BORDER_TOP; else raw_ &= ~BORDER_TOP; 
            return *this;
        }
        Attributes & setBorderRight(bool value = true) { 
            if (value) raw_ |= BORDER_RIGHT; else raw_ &= ~BORDER_RIGHT; 
            return *this;
        }
        Attributes & setBorderBottom(bool value = true) { 
            if (value) raw_ |= BORDER_BOTTOM; else raw_ &= ~BORDER_BOTTOM; 
            return *this;
        }
        Attributes & setBorderThick(bool value = true) { 
            if (value) raw_ |= BORDER_THICK; else raw_ &= ~BORDER_THICK; 
            return *this;
        }

        Attributes & setBlink(bool value = true) {
            if (value) raw_ |= BLINK; else raw_ &= ~BLINK; 
            return *this;
        }

		Attributes & clearBorder() {
			raw_ &= !(BORDER_LEFT | BORDER_TOP | BORDER_RIGHT | BORDER_BOTTOM | BORDER_THICK);
			return *this;
		}

    private:

        static constexpr uint16_t UNDERLINE = 1 << 0;
        static constexpr uint16_t STRIKETHROUGH = 1 << 1;
        static constexpr uint16_t CURLY_UNDERLINE = 1 << 2;
        static constexpr uint16_t BORDER_LEFT = 1 << 3;
        static constexpr uint16_t BORDER_TOP = 1 << 4;
        static constexpr uint16_t BORDER_RIGHT = 1 << 5;
        static constexpr uint16_t BORDER_BOTTOM = 1 << 6;
        static constexpr uint16_t BORDER_THICK = 1 << 7;
        static constexpr uint16_t BLINK = 1 << 9;

        uint16_t raw_;
    }; 


    /** Base representation of a single UI cell.  

	    Contains packed information about a single cell, namely the codepoint (unencoded Unicode), the foreground (text), background, and decorations (underlines, strikethroughs, etc) attributes (underline, borders, etc.) and the font to use to render the cell. 

		21 bits codepoint
		24 bits text (RGB)
		24 bits background (RGB)
		32 bits decoration (RGBA)
		32 bits border (RGBA)
		8 bits font (bold, italics, 8 sizes, 3bits reserved)
        16 bits decorations
		3 bits remanining (application specific, such as line end, modified, etc)

		TODO add font accessors, 

		codepointAnd

	 */

	class Cell {
	public:

		/** Default cell constructor is white space on black background. 
		 */
		Cell():
		    codepointAndFont_{0x20},
			fg_{255,255,255},
			bg_{0,0,0},
			decoration_{255,255,255},
			border_{255,255,255} {
		}

		Cell(Color fg, Color bg, char32_t codepoint):
		    codepointAndFont_{codepoint},
			decoration_{fg},
			border_{fg} {
			setFg(fg);
			setBg(bg);
		}

	    char32_t codepoint() const {
			return codepointAndFont_ & 0x1fffff;
	    }

		Font font() const {
			return pointer_cast<Font const *>(& codepointAndFont_)[3];
		}

		Attributes attributes() const {
			return attributes_;
		}

		Color fg() const {
			return Color(fg_[0], fg_[1], fg_[2]);
		}

		Color bg() const {
			return Color(bg_[0], bg_[1], bg_[2]);
		}

		Color decoration() const {
			return decoration_;
		}

		Color border() const {
			return border_;
		}

		Cell & setCodepoint(char32_t codepoint) {
			codepointAndFont_ &= ~0xffffff; // also erase the reserved bits when codepoint is set
			codepointAndFont_ |= (codepoint & 0x1fffff);
			return *this;
		}

		Cell & setFont(Font font) {
			pointer_cast<Font *>(& codepointAndFont_)[3] = font;
			return *this;
		}

		Cell & setAttributes(Attributes attrs) {
			attributes_ = attrs;
			return *this;
		}

		Cell & setFg(Color value) {
			fg_[0] = value.r;
			fg_[1] = value.g;
			fg_[2] = value.b;
			return *this;
		}

		Cell & setBg(Color value) {
			bg_[0] = value.r;
			bg_[1] = value.g;
			bg_[2] = value.b;
			return *this;
		}

		Cell & setDecoration(Color value) {
			decoration_ = value;
			return *this;
		}

		Cell & setBorder(Color value) {
			border_ = value;
			return *this;
		}

		/** \name Reserved bits
		 
		    Internally, the cell packs the unicode codepoint and the font information into single 32bit unsigned. This leaves three bits free (21 bits of unicode + 8 bits of font), which are dedicated as reserved and can be used by subclasses.
			The cell guarantees that copying & setting other things will not interfere with their value and that two cells will be identical only if their reserved bits are the same as well. 
		 */
		//@{
	    bool reserved1() const {
			return codepointAndFont_ & 0x200000;
		}

		bool reserved2() const {
			return codepointAndFont_ & 0x400000;
		}

		bool reserved3() const {
			return codepointAndFont_ & 0x800000;
		}

		Cell & setReserved1(bool value = true) {
			if (value)
			    codepointAndFont_ |= 0x200000;
			else
			    codepointAndFont_ &= ~0x200000;
			return *this;
		}

		Cell & setReserved2(bool value = true) {
			if (value)
			    codepointAndFont_ |= 0x400000;
			else
			    codepointAndFont_ &= ~0x400000;
			return *this;
		}

		Cell & setReserved3(bool value = true) {
			if (value)
			    codepointAndFont_ |= 0x800000;
			else
			    codepointAndFont_ &= ~0x800000;
			return *this;
		}
		//@}

		bool operator == (Cell const & other) const {
			return memcmp(this, &other, sizeof(Cell)) == 0;
		}

		bool operator != (Cell const & other) const {
			return memcmp(this, &other, sizeof(Cell)) != 0;
		}



		/** Returns true if the cells has text cursor in it. 
		 
		    Keeping this information in the cell allows easy tracking of whether the cursor should be displayed, or it was overlaid by some other widget and therefore should not be visible. 

			TODO delete when there is better strategy to deal with cursor placement checks
		 */
		bool isCursor() const {
			// we use the first padding bit to determine whether a cursor is at the coordinates or not. 
			return reserved3();
		}

		Cell & setCursor(bool value = true) {
			return setReserved3(value);
		}

	private:
	    char32_t codepointAndFont_;
		Attributes attributes_;
		uint8_t fg_[3];
		uint8_t bg_[3];
		Color decoration_;
		Color border_;
	}; // ui::Cell

	static_assert(sizeof(Cell) == 20, "The size of cell is expected to be packed");

} // namespace ui