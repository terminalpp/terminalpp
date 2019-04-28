#pragma once

#include <cstring>

#include "helpers/helpers.h"

namespace vterm {

	/** Different character encodings available.

		The following encodings are supported:

		ASCII - the default ASCII character set (0..127)
		UTF8 - full Unicode characters, variable length, UTF8 encoding.
		UTF16 - full Unicode characters, variable length, UTF16 encoding.
		UTF32 - full Unicode codewords, fixed length.
		UCS2 - 16bit fixed length encoding used internally by many systems. Capable of displaying only 65536 characters. 
	 */
	enum class Encoding {
		ASCII,
		UTF8,
		UTF16,
		UTF32,
	};

	/** Character representation
	 */
	class Char {
	public:
		static constexpr char BEL = 7;
		static constexpr char BACKSPACE = 8;
        static constexpr char TAB = 9;
		static constexpr char LF = 10;
		static constexpr char CR = 13;
		static constexpr char ESC = 27;

		template<Encoding E>
		class Representation;

		typedef Representation<Encoding::ASCII> ASCII;
		typedef Representation<Encoding::UTF8> UTF8;
		typedef Representation<Encoding::UTF16> UTF16;
		typedef Representation<Encoding::UTF32> UTF32;
	}; // vterm::Char


	template<>
	class Char::Representation<Encoding::ASCII> {
	public:
		//static constexpr Encoding Encoding = Encoding::ASCII;

	};

	/** UTF8 representation of character. 
	 */
	template<>
	class Char::Representation<Encoding::UTF8> {
	public:
		//static constexpr Encoding Encoding = Encoding::UTF8;

		/** Creates an UTF8 character from given ASCII char. 
		 */
		Representation(char from = 0x20) :
			bytes_{ static_cast<unsigned char>(from), 0, 0, 0 } {
		}

		/** Creates an UTF8 character from given unicode codepoint. 
		 */
		Representation(unsigned codepoint) {
			fillFromCodepoint(codepoint);
		}

		Representation(int codepoint) { // so that ambiguity from int literals is resolved
			fillFromCodepoint(codepoint);
		}

		/** Creates the UTF8 representation from given UCS2 16bit character. 
		 */
		Representation(wchar_t ucs2) {
			fillFromCodepoint(ucs2);
		}

		/** Returns the number of bytes required to encode the stored codepoint. 
		 */
		size_t size() const {
			if (bytes_[0] <= 0x7f) // 0xxxxxxx
				return 1;
			else if (bytes_[0] <= 0xdf) // 110xxxxx
				return 2;
			else if (bytes_[0] <= 0xef) // 1110xxxx
				return 3;
			else
				return 4;
		}

		/** Returns the UTF codepoint stored in the character. 
		 */
		unsigned codepoint() const {
			if (bytes_[0] <= 0x7f)
				return bytes_[0]; // 0x0xxx xxxx
			else if (bytes_[0] < 0xe0)
				return ((bytes_[0] & 0x1f) << 6) + (bytes_[1] & 0x3f); // 0x110x xxxx + 0x10xx xxxx
			else if (bytes_[0] < 0xf0)
				return ((bytes_[0] & 0x0f) << 12)  + ((bytes_[1] & 0x3f) << 6) + (bytes_[2] & 0x3f);
			else
				return ((bytes_[0] & 0x07) << 18) + ((bytes_[1] & 0x3f) << 12) + ((bytes_[2] & 0x3f) << 6) + (bytes_[3] & 0x3f);
		}

		/** Returns the codepoint stored encoded as UCS2 character. 

		    Assumes that the stored codepoint is smaller than 65536 since higher glyphs can't be stored in wchar_t. 
		 */
		wchar_t toWChar() const {
			unsigned cp = codepoint();
			ASSERT(cp < 65536) << "Unicode codepoint " << codepoint() << " cannot be encoded in single wchar_t";
			return (cp & 0xffff);
		}

		bool readFromStream(char * & buffer, char * bufferEnd) {
			unsigned char first = static_cast<unsigned char>(*buffer);
			unsigned size = 4;
			if (first < 0x80)
				size = 1;
			else if (first < 0xe0)
				size = 2;
			else if (first < 0xf0)
				size = 3;
			// check if we have enough data to read the character
			if (buffer + size > bufferEnd)
				return false;
			// copy the bytes
			std::memcpy(bytes_, buffer, size);
			buffer += size;
			return true; 
		}

		char const* rawBytes() const {
			return reinterpret_cast<char const *>(& bytes_);
		}

	private:
		/** Takes a UTF codepoint and encodes it in UTF8. 
		 */
		void fillFromCodepoint(unsigned cp) {
			if (cp < 0x80) {
				bytes_[0] = cp & 0x7f; // 0xxxxxxx
			} else if (cp < 0x800) {
				bytes_[0] = (0xc0) | ((cp >> 6) & 0x1f); // 110xxxxx
				bytes_[1] = (0x80) | (cp & 0x3f); // 10xxxxxx
			} else if (cp < 0x10000) {
				bytes_[0] = (0xe0) | ((cp >> 12) & 0x0f); // 1110xxxx
				bytes_[1] = (0x80) | ((cp >> 6) & 0x3f); // 10xxxxxx
				bytes_[2] = (0x80) | (cp & 0x3f); // 10xxxxxx
			} else {
				bytes_[0] = (0xe0) | ((cp >> 18) & 0x07); // 11110xxx
				bytes_[1] = (0x80) | ((cp >> 12) & 0x3f); // 10xxxxxx
				bytes_[2] = (0x80) | ((cp >> 6) & 0x3f); // 10xxxxxx
				bytes_[3] = (0x80) | (cp & 0x3f); // 10xxxxxx
			}
		}

		unsigned char bytes_[4];

	};

	template<>
	class Char::Representation<Encoding::UTF16> {
	public:
		//static constexpr Encoding Encoding = Encoding::UTF16;

		Representation() :
			first(0x20),
			second(0) {
		}

		/** Creates the UTF32 character from given ASCII character. 
		 */
		Representation(char from) :
			first(from),
			second(0) {
		}

		Representation(unsigned codepoint) {
			if (codepoint <= 0xd7ff || (codepoint >= 0xe000 && codepoint < 0xffff)) {
				first = codepoint;
				second = 0;
			} else {
				first = 32;
				second = 0;
				//NOT_IMPLEMENTED;
			}
		}

		size_t size() const {
			if (first <= 0xd7ff || first >= 0xe0000)
				return 1;
			else
				return 2;
		}

		wchar_t const * w_str() const {
			return & first;
		}

	private:
		wchar_t first;
		wchar_t second;

	};

	template<>
	class Char::Representation<Encoding::UTF32> {
		//static constexpr Encoding Encoding = Encoding::UTF32;

	};

} // namespace vterm


