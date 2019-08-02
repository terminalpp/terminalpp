#pragma once

namespace vterm {

	/** Describes a font for the vterminal purposes. 

	 */
	class Font {
	public:

		/** Returns the size of the font. 

		    Minimum size is 1. 
		 */
		unsigned size() const {
			return bits_.size + 1;
		}

		/** Returns true if the font is bold. 
		 */
		bool bold() const {
			return bits_.bold;
		}

		/** Returns true if the font is italics. 
		 */
		bool italics() const {
			return bits_.italics;
		}

		/** Returns true if the font is underlined. 
		 */
		bool underline() const {
			return bits_.underline;
		}

		/** Returns true if the font is striked through. 
		 */
		bool strikethrough() const {
			return bits_.strikethrough;
		}

		/** Returns true if the font should blink. 
		 */
		bool blink() const {
			return bits_.blink;
		}

		void setSize(unsigned size) {
			ASSERT(size >= 1 && size <= 8);
			bits_.size = size - 1;
		}

		void setBold(bool value) {
			bits_.bold = value;
		}

		void setItalics(bool value) {
			bits_.italics = value;
		}

		void setUnderline(bool value) {
			bits_.underline = value;
		}

		void setStrikethrough(bool value) {
			bits_.strikethrough = value;
		}

		void setBlink(bool value) {
			bits_.blink = value;
		}

		unsigned char raw() const {
			return raw_;
		}

		Font() {
			raw_ = 0;
		}

		Font(unsigned size, bool bold, bool italics, bool underline, bool strikethrough, bool blink) {
			bits_.size = size;
			bits_.bold = bold;
			bits_.italics = italics;
			bits_.underline = underline;
			bits_.strikethrough = strikethrough;
			bits_.blink = blink;
		}

		bool operator == (Font const & other) const {
			return raw_ == other.raw_;
		}

		bool operator != (Font const & other) const {
			return raw_ != other.raw_;
		}

	private:
		friend struct ::std::hash<Font>;

		struct FontBits {
			unsigned size : 3;
			unsigned bold : 1;
			unsigned italics : 1;
			unsigned underline : 1;
			unsigned strikethrough : 1;
			unsigned blink : 1;
		} ;

		/** Note that the code regularly assumes that the width of raw font description is 1 byte.
		 */
		union {
			FontBits bits_;
			unsigned char raw_;
		};
	}; // vterm::Font


} // namespace vterm

