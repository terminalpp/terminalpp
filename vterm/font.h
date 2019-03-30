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
			return size_ + 1;
		}

		/** Returns true if the font is bold. 
		 */
		bool bold() const {
			return bold_;
		}

		/** Returns true if the font is italics. 
		 */
		bool italics() const {
			return italics_;
		}

		/** Returns true if the font is underlined. 
		 */
		bool underline() const {
			return underline_;
		}

		/** Returns true if the font is striked out. 
		 */
		bool strikeout() const {
			return strikeout_;
		}

		/** Returns true if the font should blink. 
		 */
		bool blink() const {
			return blink_;
		}

		void setBold(bool value) {
			bold_ = value;
		}

		void setItalics(bool value) {
			italics_ = value;
		}

		void setUnderline(bool value) {
			underline_ = value;
		}

		void setStrikeout(bool value) {
			strikeout_ = value;
		}

		void setBlink(bool value) {
			blink_ = value;
		}

		unsigned char raw() const {
			return raw_;
		}

		Font() :
			size_(0),
			bold_(0),
			italics_(0),
			underline_(0),
			strikeout_(0),
		    blink_(0) {
		}

		Font(unsigned size, bool bold, bool italics, bool underline, bool strikeout, bool blink) :
			size_(size),
			bold_(bold),
			italics_(italics),
			underline_(underline),
			strikeout_(strikeout),
		    blink_(blink) {
		}

		bool operator == (Font const & other) const {
			return raw_ == other.raw_;
		}

		bool operator != (Font const & other) const {
			return raw_ != other.raw_;
		}

	private:
		friend struct ::std::hash<Font>;

		/** Note that the code regularly assumes that the width of raw font description is 1 byte. 
		 */
		union {
			struct {
				unsigned size_ : 3; 
				unsigned bold_ : 1;
				unsigned italics_ : 1;
				unsigned underline_ : 1;
				unsigned strikeout_ : 1;
				unsigned blink_ : 1;
			};
			unsigned char raw_;
		};
	}; // vterm::Font


} // namespace vterm

