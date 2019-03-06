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

		void setBold(bool value) {
			bold_ = value;
		}

		Font() :
			size_(0),
			bold_(0),
			italics_(0),
			underline_(0),
			strikeout_(0) {
		}

		Font(unsigned size, bool bold, bool italics, bool underline, bool strikeout) :
			size_(size),
			bold_(bold),
			italics_(italics),
			underline_(underline),
			strikeout_(strikeout) {
		}

		bool operator == (Font const & other) const {
			return raw_ == other.raw_;
		}

		bool operator != (Font const & other) const {
			return raw_ != other.raw_;
		}

		/*
		struct BlinkIgnoringHash {
			size_t operator () (Font const & x) const {
				return std::hash<unsigned char>()(x.raw_ & 0xfe);
			}
		};

		struct BlinkIgnoringComparator {
			bool operator () (Font const & a, Font const & b) const {
				return (a.raw_ & 0xfe) == (b.raw_ & 0xfe);
			}
		}; */

	private:
		friend struct ::std::hash<Font>;
		/*
		friend struct BlinkIgnoringHash;
		friend struct BlinkIgnoringComparator;
		*/

		union {
			struct {
				unsigned size_ : 3; 
				unsigned bold_ : 1;
				unsigned italics_ : 1;
				unsigned underline_ : 1;
				unsigned strikeout_ : 1;
			};
			unsigned char raw_;
		};
	}; // vterm::Font


} // namespace vterm

namespace std {
	template<>
	struct hash<::vterm::Font> {
		size_t operator() (::vterm::Font const & x) const {
			return std::hash<unsigned char>()(x.raw_);
		}
	};

} // namespace std

