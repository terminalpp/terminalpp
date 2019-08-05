#pragma once

#include <cstdint>

namespace ui {

	/** Describes a font. 
	 
	    Determines the size (1-8) of the font and whether it is bold, or italics.  
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
		Font(unsigned size, bool bold, bool italics):
		    raw_(size & 0x7) {
			if (bold)
			    raw_ |= BOLD;
			if (italics)
			    raw_ |= ITALICS;
		}

	    /** Returns the size of the font. 
		 
		    Sizes from 1 to 8 are supported. 
		 */
	    unsigned size() const {
			return (raw_ & 0x7) + 1;
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

		bool operator == (Font const & other) const {
			return raw_ == other.raw_;
		}

		bool operator != (Font const & other) const {
			return raw_ != other.raw_;
		}

	private:
	    friend class Cell;

		static constexpr unsigned BOLD = 0x08;
		static constexpr unsigned ITALICS = 0x10;

		Font(uint8_t raw):
		    raw_(raw) {
		}

	    uint8_t raw_;

	}; // ui::Font



} // namespace ui