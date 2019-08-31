#pragma once

#include <cstdint>

#include "helpers/helpers.h"

namespace ui {

	/** Describes a font. 
	 
	    A font is described by its style (regular, bold, italics, bold + italics) and size. The size of the font is measured in cells and both width and height can be set with the following effects:

		The renderer will first determine the appropriate font and size so that it would fit best into the given area. It would then center the glyph in the area and render it, advancing by the specified number of cells. If the font is the same as the base font, then increasing both width and height will render larger letters, increasing one or another will increase spacing. 

		If a different font will be used with more rectangular characters, such as icons & CJK characters, then keeping the height to 1, but increasing the width is likely to render larger glyphs that still fit in the default line. 
	 */
    class Font {
	public:

		/** Creates default font, i.e. size 1 and neither bold, nor italics. 
  		 */
        Font():
		    raw_(0) {
		}

		/** Creates a font of specific properties. 
  		 */
		Font(bool bold, bool italics):
		    raw_(0) {
			if (bold)
			    raw_ |= BOLD;
			if (italics)
			    raw_ |= ITALICS;
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

		/** Returns the width of the font in cells. 
		 */
		unsigned width() const {
			return (raw_ & 0x7) + 1;
		}

		/** Returns the height of the font in cells. 
		 */
		unsigned height() const {
			return ((raw_ & 0x38) >> 3) + 1;
		}

		/** Sets the width of the font in cells. 
		 
		    Values from 1 to 8 inclusive are supported. 
		 */
		Font & setWidth(unsigned width) {
			ASSERT(width <= 8);
			raw_ = (raw_ & 0xf8) + ((width & 0xf) - 1);
			return *this;
		}

		/** Sets the width of the font in cells. 
		 
		    Values from 1 to 8 inclusive are supported. 
		 */
		Font & setHeight(unsigned height) {
			ASSERT(height <= 8);
			raw_ = (raw_ & 0xc7) + (((height & 0xf) - 1) << 3);
			return *this;
		}

		bool operator == (Font const & other) const {
			return raw_ == other.raw_;
		}

		bool operator != (Font const & other) const {
			return raw_ != other.raw_;
		}

	private:
	    friend class Cell;

		static constexpr unsigned BOLD = 0x40;
		static constexpr unsigned ITALICS = 0x80;

		Font(uint8_t raw):
		    raw_(raw) {
		}

	    uint8_t raw_;

	}; // ui::Font



} // namespace ui