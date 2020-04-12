#pragma once

#include <cstdint>

#include "helpers/helpers.h"

namespace ui {

	/** Describes a font. 
	 
	    A font is described by its style, i.e. whether it is bold and/or italics, its size (in terms of the base cells, i.e. font of size 1 has width of 1 cell width and height of 1 cell height, font of size 2 is 2 cells width and 2 cells height) and whether the font is double width font, i.e. its width is twice as many cell widths as its height is cell heights. 
	 */
    class Font {
	public:

		/** Creates default font, i.e. width and height 1 and neither bold, nor italics. 
  		 */
        Font():
		    raw_(0) {
		}

		/** Creates a font of specific properties. 
  		 */
		Font(bool bold, bool italics, bool doubleWidth = false):
		    raw_(0) {
			if (bold)
			    raw_ |= BOLD;
			if (italics)
			    raw_ |= ITALICS;
			if (doubleWidth)
			    raw_ |= DOUBLE_WIDTH;
		}

		/** Creates a font with given properties. 
		 */
		Font(bool bold, bool italics, unsigned size, bool doubleWidth = false):
		    raw_(0) {
			setBold(bold);
			setItalics(italics);
			setDoubleWidth(doubleWidth);
			setSize(size);
		}

		/** Returns the size of the font, in multiplies of the default cell. 
		 */
		unsigned size() const {
			return (raw_ & 1) + 1;
		}

		/** Returns true if the font is bold. 
 		 */
		bool bold() const {
			return raw_ & BOLD;
		}

		/** Returns true if the font is italics.  
  		 */
		bool italics() const {
			return raw_ & ITALICS;
		}

		/** Determines whether double width font should be used. 
		 */
		bool doubleWidth() const {
			return raw_ & DOUBLE_WIDTH;
		}

		/** Sets the size of the font. 
		 */
		Font & setSize(unsigned value) {
			ASSERT(value <= 2);
			raw_ = (raw_ & 0xfe) + static_cast<int8_t>(value - 1);
			return *this;
		}

		/** Sets whether the font is bold, or not. 
    	 */
		Font & setBold(bool value = true) {
			if (value)
			    raw_ |= BOLD;
			else
			    raw_ &= ~ BOLD;
			return *this;
		}

		/** Sets whether the font is in italics, or not. 
    	 */
		Font & setItalics(bool value = true) {
			if (value)
			    raw_ |= ITALICS;
			else
			    raw_ &= ~ ITALICS;
			return *this;
		}

		/** Sets whether double width font should be used or not. 
		 */
		Font & setDoubleWidth(bool value = true) {
			if (value)
			    raw_ |= DOUBLE_WIDTH;
			else
			    raw_ &= ~ DOUBLE_WIDTH;
			return *this;
		}

	    int width() const {
			if (doubleWidth())
			    return size() * 2;
			else
			    return size();
		}

		int height() const {
			return size();
		}

		bool operator == (Font const & other) const {
			return raw_ == other.raw_;
		}

		bool operator != (Font const & other) const {
			return raw_ != other.raw_;
		}

	private:
	    friend class Cell;

		static constexpr unsigned BOLD = 0x80;
		static constexpr unsigned ITALICS = 0x40;
		static constexpr unsigned DOUBLE_WIDTH = 0x20;

		Font(uint8_t raw):
		    raw_(raw) {
		}

	    uint8_t raw_;

	}; // ui::Font



} // namespace ui