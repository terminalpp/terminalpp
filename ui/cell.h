#pragma once

#include <cstdint>

#include "helpers/helpers.h"

#include "color.h"
#include "font.h"

namespace ui {
    /** Base representation of a single UI cell.  

	    Contains packed information about a single cell, namely the codepoint (unencoded Unicode), the foreground (text), background, and decorations (underlines, strikethroughs, etc) attributes (underline, borders, etc.) and the font to use to render the cell. 

		21 bits codepoint
		24 bits text (RGB)
		24 bits background (RGB)
		32 bits decoration (RGBA)
		8 bits font (bold, italics, 8 sizes, 3bits reserved)
        16 bits decorations
		3 bits remanining (application specific, such as line end, modified, etc)

		TODO add font accessors, 
	 */
    class Cell {
	public:
    	enum class Attributes {
			Underline,
			Strikethrough,
			CurlyUnderline,
			BorderLeft,
			BorderTop,
			BorderRight,
			BorderBottom,
			BorderThick,
			BorderAbove,

			EndOfLine
	    }; // Cell::Attributes

		/** Default cell constructor is white space on black background. 
		 */
		Cell():
			big_{ 0xffffff0000000020, 0xffffffff00000000 } {
			static_assert(sizeof(Cell) == 16, "Invalid cell size, padding must be adjusted");
		}

		/** Returns the codepoint stored in the cell. 
		 */
	    char32_t codepoint() const {
			return small_[0] & 0x1fffff;
		}

		/** Returns the font of the cell. 
 		 */
		Font font() const {
			return Font(small_[0] >> 24);
		}

		/** Returns the foreground color of the cell. 
		 
		    Note that tghe foreground color's opacity is always 255. 
		 */
		Color foreground() const {
			return Color(small_[1] | 0xff);
		}

		/** Returns the background color of the cell. 
		 
		    Note that the background color's opacity is always 255.
		 */
		Color background() const {
			return Color(small_[2] | 0xff);
		}

		Color decorationColor() const {
			return Color(small_[3]);
		}

		bool underline() const {
			return small_[1] & UNDERLINE;
		}

		bool strikethrough() const {
			return small_[1] & STRIKETHROUGH;
		}

		bool curlyUnderline() const {
			return small_[1] & CURLY_UNDERLINE;
		}

		bool borderLeft() const {
			return small_[1] & BORDER_LEFT;
		}

		bool borderTop() const {
			return small_[1] & BORDER_TOP;
		}

		bool borderRight() const {
			return small_[1] & BORDER_RIGHT;
		}

		bool borderBottom() const {
			return small_[1] & BORDER_BOTTOM;
		}

		bool borderThick() const {
			return small_[1] & BORDER_THICK;
		}

		bool borderAbove() const {
			return small_[2] & BORDER_ABOVE;
		}

		bool endOfLine() const {
			return small_[2] & END_OF_LINE;
		}


		Cell & operator = (Cell const & other) {
			big_[0] = other.big_[0];
			big_[1] = other.big_[1];
			return *this;
		}

		bool operator == (Cell const & other) const {
			return big_[0] == other.big_[0] && big_[1] == other.big_[1];
		}

		bool operator != (Cell const & other) const {
			return big_[0] != other.big_[0] || big_[1] != other.big_[1];
		}

		/** Returns true is the attributes of the given cell are identical to visible own visible attributes.
		   
		    TODO what are visible attributes
		 */
		bool sameVisibleAttributesAs(Cell const & other) const {
			return ((small_[1] & 0xff) == (other.small_[1] & 0xff)) && ((small_[2] & 0xff) == (other.small_[2] & 0xff));
		}

    protected:

	    unsigned padding() const {
			return small_[0] >> 21 & 0x7;
		}

		void setPadding(unsigned value) {
			ASSERT(value <= 7) << "Only three bits of padding are available";
			small_[0] &= 0xff1fffff;
			small_[0] |= (value << 21);
		}

	private:
		// first part
		static constexpr uint32_t UNDERLINE = 1 << 0;
		static constexpr uint32_t STRIKETHROUGH = 1 << 1;
		static constexpr uint32_t CURLY_UNDERLINE = 1 << 2;
		static constexpr uint32_t BORDER_LEFT = 1 << 3;
		static constexpr uint32_t BORDER_TOP = 1 << 4;
		static constexpr uint32_t BORDER_RIGHT = 1 << 5;
		static constexpr uint32_t BORDER_BOTTOM = 1 << 6;
		static constexpr uint32_t BORDER_THICK = 1 << 7;

		// second part
		static constexpr uint32_t BORDER_ABOVE = 1 << 0;

		static constexpr uint32_t END_OF_LINE = 1 << 7;


	    /** Sets the codepoint of the cell. 
 		 */
		template<typename CELL>
		friend typename std::enable_if<std::is_base_of<Cell, CELL>::value, CELL &>::type operator << (CELL & cell, char32_t codepoint) {
			cell.small_[0] &= 0xff700000;
			cell.small_[0] |= codepoint & 0x1fffff;
			return cell;
		}

		/** Sets the font of the cell. 
 		 */
		template<typename CELL>
		friend typename std::enable_if<std::is_base_of<Cell, CELL>::value, CELL &>::type operator << (CELL & cell, Font font) {
			cell.small_[0] &= 0xffffff;
			cell.small_[0] |= (*helpers::pointer_cast<unsigned char *>(&font)) << 24;
			return cell;
		}

		/** Foreground color */
		template<typename CELL>
		friend typename std::enable_if<std::is_base_of<Cell, CELL>::value, CELL &>::type operator << (CELL & cell, ForegroundColorHolder color) {
			cell.small_[1] &= 0xff;
			cell.small_[1] |= (color.value.toRGB() << 8);
			return cell;
		}

		/** Background color */
		template<typename CELL>
		friend typename std::enable_if<std::is_base_of<Cell, CELL>::value, CELL &>::type operator << (CELL & cell, BackgroundColorHolder color) {
			cell.small_[2] &= 0xff;
			cell.small_[2] |= (color.value.toRGB() << 8);
			return cell;
		}

		/** Decorations color */
		template<typename CELL>
		friend typename std::enable_if<std::is_base_of<Cell, CELL>::value, CELL &>::type operator << (CELL & cell, DecorationColorHolder color) {
			cell.small_[3] = color.value.toRGBA();
			return cell;
		}

		/** Sets the attributes of the cell. 
    	 */
		template<typename CELL>
		friend typename std::enable_if<std::is_base_of<Cell, CELL>::value, CELL &>::type operator += (CELL & cell, Attributes attr) {
			switch (attr) {
				case Cell::Attributes::Underline:
				    cell.small_[1] |= Cell::UNDERLINE;
					break;
				case Cell::Attributes::Strikethrough:
				    cell.small_[1] |= Cell::STRIKETHROUGH;
					break;
				case Cell::Attributes::CurlyUnderline:
				    cell.small_[1] |= Cell::CURLY_UNDERLINE;
					break;
				case Cell::Attributes::BorderLeft:
				    cell.small_[1] |= Cell::BORDER_LEFT;
					break;
				case Cell::Attributes::BorderTop:
				    cell.small_[1] |= Cell::BORDER_TOP;
					break;
				case Cell::Attributes::BorderRight:
				    cell.small_[1] |= Cell::BORDER_RIGHT;
					break;
				case Cell::Attributes::BorderBottom:
				    cell.small_[1] |= Cell::BORDER_BOTTOM;
					break;
				case Cell::Attributes::BorderThick:
				    cell.small_[1] |= Cell::BORDER_THICK;
					break;
				case Cell::Attributes::BorderAbove:
				    cell.small_[2] |= Cell::BORDER_ABOVE;
					break;
				case Cell::Attributes::EndOfLine:
				    cell.small_[2] |= Cell::END_OF_LINE;
					break;
				default:
				    UNREACHABLE;
			}
			return cell;
		}

		/** Clears the attributes of the cell. 
 		 */
		template<typename CELL>
		friend typename std::enable_if<std::is_base_of<Cell, CELL>::value, CELL &>::type operator -= (CELL & cell, Attributes attr) {
			switch (attr) {
				case Cell::Attributes::Underline:
				    cell.small_[1] &= ~ Cell::UNDERLINE;
					break;
				case Cell::Attributes::Strikethrough:
				    cell.small_[1] &= ~ Cell::STRIKETHROUGH;
					break;
				case Cell::Attributes::CurlyUnderline:
				    cell.small_[1] &= ~ Cell::CURLY_UNDERLINE;
					break;
				case Cell::Attributes::BorderLeft:
				    cell.small_[1] &= ~ Cell::BORDER_LEFT;
					break;
				case Cell::Attributes::BorderTop:
				    cell.small_[1] &= ~ Cell::BORDER_TOP;
					break;
				case Cell::Attributes::BorderRight:
				    cell.small_[1] &= ~ Cell::BORDER_RIGHT;
					break;
				case Cell::Attributes::BorderBottom:
				    cell.small_[1] &= ~ Cell::BORDER_BOTTOM;
					break;
				case Cell::Attributes::BorderThick:
				    cell.small_[1] &= ~ Cell::BORDER_THICK;
					break;
				case Cell::Attributes::BorderAbove:
				    cell.small_[2] &= ~ Cell::BORDER_ABOVE;
					break;
				case Cell::Attributes::EndOfLine:
				    cell.small_[2] &= ~ Cell::END_OF_LINE;
					break;
				default:
				    UNREACHABLE;
			}
			return cell;
		}

	    union {
			uint64_t big_[2];
			uint32_t small_[4];
		};
	}; // ui::Cell

} // namespace ui