#pragma once

#include <ostream>

#include "helpers.h"

#ifdef ARCH_WINDOWS
static_assert(sizeof(wchar_t) == sizeof(char16_t), "wchar_t and char16_t must have the same size or the conversions would break");
#endif


namespace helpers {

// TODO add and use utf8_string and utf32 string and chars?

	typedef std::basic_stringstream<char> utf8_stringstream;
#ifdef ARCH_WINDOWS
	typedef wchar_t utf16_char;
	typedef std::wstring utf16_string;
	typedef std::wstringstream utf16_stringstream;
#else
	typedef char16_t utf16_char;
	typedef std::u16string utf16_string;
	typedef std::basic_stringstream<char16_t> utf16_stringstream;
#endif

	class CharError : public IOError {
	};

	/** UTF8 character representation. 
	 */
	class Char {
	public:

		static constexpr char NUL = 0;
		static constexpr char BEL = 7;
		static constexpr char BACKSPACE = 8;
		static constexpr char TAB = 9;
		static constexpr char LF = 10;
		static constexpr char CR = 13;
		static constexpr char ESC = 27;

		Char(char c = ' ') :
			bytes_{ static_cast<unsigned char>(c), 0, 0, 0 } {
			ASSERT(c >= 0) << "ASCII out of range";
		}

		Char(Char const& from) = default;

		Char & operator = (Char const & from) = default;

        bool operator == (Char const & other) const {
            for (size_t i = 0; i < 4; ++i)
                if (bytes_[i] != other.bytes_[i])
                    return false;
            return true;
        }

        bool operator != (Char const & other) const {
            for (size_t i = 0; i < 4; ++i)
                if (bytes_[i] != other.bytes_[i])
                    return true;
            return false;
        }

		/** Creates the character from given ASCII character. 
		 */
		static Char FromASCII(char x) {
			return FromCodepoint(x);
		}

		/** Creates the character from given UTF16 character stream, advancing the stream accordingly. 

		    Surrogate pairs and unpaired surrogates are supported. 
		 */
		static Char FromUTF16(utf16_char const *& x, utf16_char const * end) {
			if (x >= end)
				THROW(CharError()) << "Cannot read character, buffer overflow";
			// range < 0xd800 and >= 0xe000 are the codepoints themselves, single byte
			if (*x < 0xd800 || *x >= 0xe000)
				return FromCodepoint(*x++);
			// otherwise the stored value is a codepoint which is above 0x10000
			char32_t cp = 0;
			// if the first byte is > 0xd800 and < 0xdc00, it is either high byte of surrogate pair, or possible unpaired high surrogate
			if (*x >= 0xd800 && *x < 0xdc00) // high-end surrogate pair, or high unpaired surrogate
				cp = static_cast<unsigned>(*(x++) - 0xd800) << 10;
			// now see if there is low surrogate (this happen either after high surrogate already parsed, or if the codepoint is low surrogate alone can be the first thing we see
			if (x != end && (*x >= 0xdc00 && *x < 0xdfff)) // low end surrogate pair or low unpaired surrogate
				cp += (*(x++) - 0xdc00);
			// don't forget to increment the codepoint
			return FromCodepoint(cp + 0x10000);
		}

		/** Creates the character from given unicode codepoint. 
		 */
		static Char FromCodepoint(char32_t cp) {
			Char result;
			result.fillFromCodepoint(cp);
			return result;
		}

		static Char const* At(char*& buffer, char const* bufferEnd) {
			return At(const_cast<char const*&>(buffer), bufferEnd);
		}

		/** Given a pointer to valid UTF8 character encoded in char array, returns a Char around it. 
		
		 */
		static Char const * At(char const * & buffer, char const * bufferEnd) {
			if (buffer == bufferEnd)
				return nullptr;
			unsigned char const * start = reinterpret_cast<unsigned char const *>(buffer);
			if (*start >= 0x80) {
				if (*start < 0xe0) {
					if (buffer + 2 > bufferEnd)
						return nullptr;
					buffer += 2;
				} else if (*start < 0xf0) {
					if (buffer + 3 > bufferEnd)
						return nullptr;
					buffer += 3;
				} else {
					if (buffer + 4 > bufferEnd)
						return nullptr;
					buffer += 4;
				}
			} else {
				++buffer;
			}
			return reinterpret_cast<Char const*>(start);
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

		/** Returns the unicode codepoint stored by the character. 
		 */
		char32_t codepoint() const {
			if (bytes_[0] <= 0x7f)
				return bytes_[0]; // 0x0xxx xxxx
			else if (bytes_[0] < 0xe0)
				return ((bytes_[0] & 0x1f) << 6) + (bytes_[1] & 0x3f); // 0x110x xxxx + 0x10xx xxxx
			else if (bytes_[0] < 0xf0)
				return ((bytes_[0] & 0x0f) << 12) + ((bytes_[1] & 0x3f) << 6) + (bytes_[2] & 0x3f);
			else
				return ((bytes_[0] & 0x07) << 18) + ((bytes_[1] & 0x3f) << 12) + ((bytes_[2] & 0x3f) << 6) + (bytes_[3] & 0x3f);
		}

// macOS does not really support the C++ standard wrt char16_t. Fportunately we only need it on Windows for now
#ifndef ARCH_MACOS
		void toUTF16(utf16_stringstream& s) const {
			unsigned cp = codepoint();
			if (cp < 0x10000) {
				ASSERT(cp < 0xd800 || cp >= 0xe000) << "Invalid UTF16 codepoint";
				s << static_cast<utf16_char>(cp);
			} else {
				cp -= 0x10000;
				unsigned high = cp >> 10; // upper 10 bits
				unsigned low = cp & 0x3ff; // lower 10 bits
				s << static_cast<utf16_char>(high + 0xd800);
				s << static_cast<utf16_char>(low + 0xdc00);
			}
		}
#endif

		char const* toCharPtr() const {
			return reinterpret_cast<char const*>(&bytes_);
		}

		/** Returns number of columns in monospace font (such as a terminal) the given character should occupy. 

		    On linux, the funtion wcwidth should do, however this does not exist on Windows and to make sure that applications behave the same on all platforms, own decission is implemented. 
		 */
		int columnWidth() const {
			return ColumnWidth(codepoint());
        }

        static int ColumnWidth(char32_t cp) {
			if (cp >= 0x1100) {
				if (cp <= 0x115f || // Hangul
				    cp == 0x2329 ||
					cp == 0x2321 ||
					(cp >= 0x2e80 && cp <= 0xa4cf && cp != 0x303f) || // CJK
					(cp >= 0xac00 && cp <= 0xd7a3) || // Hangul Syllables
					(cp >= 0xf900 && cp <= 0xfaff) || // CJK
					(cp >= 0xfe10 && cp <= 0xfe19) || // Vertical forms
					(cp >= 0xfe30 && cp <= 0xfe6f) || // CJK
					(cp >= 0xff00 && cp <= 0xff60) || // Fullwidth forms
					(cp >= 0xffe0 && cp <= 0xffe6) ||
					(cp >= 0x20000 && cp <= 0x2fffd) ||
					(cp >= 0x30000 && cp <= 0x3fffd))
					return 2;
			}
			return 1;
		}

	private:

		friend std::ostream& operator << (std::ostream& s, Char c) {
			s.write(reinterpret_cast<char const *>(&c.bytes_), c.size());
			return s;
		}

		void fillFromCodepoint(char32_t cp) {
			if (cp < 0x80) {
				bytes_[0] = cp & 0x7f; // 0xxxxxxx
				bytes_[1] = 0;
				bytes_[2] = 0;
				bytes_[3] = 0;
			} else if (cp < 0x800) {
				bytes_[0] = (0xc0) | ((cp >> 6) & 0x1f); // 110xxxxx
				bytes_[1] = (0x80) | (cp & 0x3f); // 10xxxxxx
				bytes_[2] = 0;
				bytes_[3] = 0;
			} else if (cp < 0x10000) {
				bytes_[0] = (0xe0) | ((cp >> 12) & 0x0f); // 1110xxxx
				bytes_[1] = (0x80) | ((cp >> 6) & 0x3f); // 10xxxxxx
				bytes_[2] = (0x80) | (cp & 0x3f); // 10xxxxxx
				bytes_[3] = 0;
			} else {
				bytes_[0] = (0xe0) | ((cp >> 18) & 0x07); // 11110xxx
				bytes_[1] = (0x80) | ((cp >> 12) & 0x3f); // 10xxxxxx
				bytes_[2] = (0x80) | ((cp >> 6) & 0x3f); // 10xxxxxx
				bytes_[3] = (0x80) | (cp & 0x3f); // 10xxxxxx
			}
		}


		unsigned char bytes_[4];
	}; // helpers::Char

	inline bool IsDecimalDigit(char what) {
		return InRangeInclusive(what, '0', '9');
	}

	/** Determines if given character is a hexadecimal digit. 
	 
	    Numbers and letters `a`-`f` and `A`-`F` are recognized as hexadecimal digits. 
	 */
	inline bool IsHexadecimalDigit(char what) {
		return InRangeInclusive(what, '0', '9') || InRangeInclusive(what, 'a', 'f') || InRangeInclusive(what, 'A', 'F');
	}

	/** Determines whether given character whitespace or not. 

	    New line, carriage return, space and tab are considered whitespace characters. 
	 */
	inline bool IsWhitespace(char what) {
		return what == ' ' || what == '\t' || what == '\r' || what == '\n';
	}

	/** Converts given character containing a decimal digit to its value.
	 */
	inline unsigned DecCharToNumber(char what) {
		ASSERT(InRangeInclusive(what, '0', '9')) << "Not a decimal number: " << what << " (ASCII: " << static_cast<unsigned>(what) << ")";
		return what - '0';
	}

	/** Converts given character containing hexadecimal digit (upper and lower case variants of a-f are supported) to its value.
	 */
	inline unsigned HexCharToNumber(char what) {
		ASSERT(InRangeInclusive(what, '0', '9') || InRangeInclusive(what, 'A', 'F') || InRangeInclusive(what, 'a', 'f')) << "Not a hexadecimal number: " << what << " (ASCII: " << static_cast<unsigned>(what) << ")";
		if (what <= '9')
			return what - '0';
		if (what <= 'F')
			return what - 'A' + 10;
		return
			what - 'a' + 10;
	}

	/** Parses a number from selected number of hexadecimal digits. 
	 */
	inline unsigned ParseHexNumber(char const * what, size_t numDigits) {
		unsigned result = 0;
		while (numDigits-- > 0) {
			if (!IsHexadecimalDigit(*what))
			    THROW(IOError()) << "Expected hexadecimal digit, but " << *what << " found.";
			result = result * 16 + HexCharToNumber(*what);
			++what; 
		}
		return result;
	}

	/** Converts given number in range 0-15 to a hex digit (0..9a..f).
	 */
	inline char ToHexDigit(unsigned what) {
		ASSERT(what < 16) << "Value " << what << " too large for single hex digit";
		if (what < 10)
			return static_cast<char>('0' + what);
		else
			return static_cast<char>('a' + what - 10);
	}




} // namespace helpers