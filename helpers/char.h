#pragma once

#include <ostream>

#include "helpers.h"

#ifdef ARCH_WINDOWS
static_assert(sizeof(wchar_t) == sizeof(char16_t), "wchar_t and char16_t must have the same size or the conversions would break");
#endif


HELPERS_NAMESPACE_BEGIN

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

		/** Iterator into char const * that does UTF8 encoding. 
		 
		    Allows forwards and backwards iteration over UTF8 strings. 
		 */
	    class iterator_utf8 {
		public:

            typedef Char value_type;

			iterator_utf8():
			    i_{} {
			}

			/** Creates the UTF8 iterator from an std::string iterator. 
			 */
		    iterator_utf8(std::string::const_iterator const & i):
			    i_{pointer_cast<unsigned char const *>(&*i)} {
			}

			iterator_utf8(char const * from):
    			i_{pointer_cast<unsigned char const *>(from)} {
			}

			iterator_utf8(iterator_utf8 const & from):
			    i_{from.i_} {
			}

			iterator_utf8 & operator = (iterator_utf8 const & from) {
				i_ = from.i_;
				return *this;
			}

			bool operator == (iterator_utf8 const & other) const {
				return i_ == other.i_;
			}

			bool operator != (iterator_utf8 const & other) const {
				return i_ != other.i_;
			}

			bool operator < (iterator_utf8 const & other) const {
				return i_ < other.i_;
			}

			bool operator > (iterator_utf8 const & other) const {
				return i_ > other.i_;
			}

			bool operator <= (iterator_utf8 const & other) const {
				return i_ <= other.i_;
			}
			bool operator >= (iterator_utf8 const & other) const {
				return i_ >= other.i_;
			}

			/** Returns the UTF8 character to which the iterator points. 
			 
                Note that the character does not really exist in memory and especially towards the end of the string array, some of the character bytes can be outside of the allocated area. 
                
                This should be ok as long as the string is valid UTF8 as the const functions of the character only ever access the valid bytes of the character.
			 */
			Char const & operator * () const {
                return * pointer_cast<Char const *>(i_);
			}

            /** Returns the UTF8 character to which the iterator points. 
             
                Note that the character does not really exist in memory and especially towards the end of the string array, some of the character bytes can be outside of the allocated area. 
                
                This should be ok as long as the string is valid UTF8 as the const functions of the character only ever access the valid bytes of the character.
             */
            Char const * operator -> () const {
                return pointer_cast<Char const *>(i_);
            }

			size_t charSize() const {
				unsigned char x = static_cast<unsigned char>(*i_);
				if (x < 0x80) 
				    return 1;
				else if (x < 0xe0)
				    return 2;
				else if (x < 0xf0)
				    return 3;
				else 
				    return 4;
			}

			/** Prefix increment. 
			 */
		    iterator_utf8 & operator ++ () {
				i_ += charSize();
				return *this;
			}

			/** Postfix increment. 
			 */
			iterator_utf8 operator ++ (int) {
				iterator_utf8 result{*this};
				++(*this);
				return result;
			}

			/** Prefix decrement. 
			 */
			iterator_utf8 & operator -- () {
				unsigned char const * old = i_;
				--i_;
				while ((*i_ & 0xc0) == 0x80) {
				    --i_;
					if (old - i_ > 4)
					    THROW(CharError()) << "Not UTF8 encoding";
				}
				return *this;
			}

			/** Postrfix decrement. 
		     */
			iterator_utf8 operator -- (int) {
				iterator_utf8 result{*this};
				--(*this);
				return result;
			}

			iterator_utf8 & operator += (size_t offset) {
				while (offset-- > 0)
				    ++(*this);
				return *this;
			}

			iterator_utf8 & operator -= (size_t offset) {
				while (offset-- > 0)
				    --(*this);
				return *this;
			}

			iterator_utf8 & operator += (int offset) {
				if (offset > 0) {
					while (--offset > 0)
					    ++(*this);
				} else if (offset < 0) {
					while (++offset < 0)
					    --(*this);
			    }
				return *this;
			}

			iterator_utf8 & operator -= (int offset) {
				if (offset > 0) {
					while (--offset > 0)
					    --(*this);
				} else if (offset < 0) {
					while (++offset < 0)
					    ++(*this);
			    }
				return *this;
			}

		private:
		    unsigned char const * i_;

		}; // Char::iterator_utf8

		static iterator_utf8 BeginOf(std::string const & str) {
			return iterator_utf8{str.c_str()};
		}

		static iterator_utf8 BeginOf(std::string_view const & str) {
			return iterator_utf8{str.data()};
		}

		static iterator_utf8 EndOf(std::string const & str) {
			return iterator_utf8{str.c_str() + str.size()};
		}
		static iterator_utf8 EndOf(std::string_view const & str) {
			return iterator_utf8{str.data() + str.size()};
		}

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

        /** Creates the character from an unencoded (UTF32) 32bit codepoint. 
         */
        Char(char32_t codepoint) {
            fillFromCodepoint(codepoint);
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
			return Char{static_cast<char32_t>(x)};
		}

		/** Creates the character from given UTF16 character stream, advancing the stream accordingly. 

		    Surrogate pairs and unpaired surrogates are supported. 
		 */
		static Char FromUTF16(utf16_char const *& x, utf16_char const * end) {
			if (x >= end)
				THROW(CharError()) << "Cannot read character, buffer overflow";
			// range < 0xd800 and >= 0xe000 are the codepoints themselves, single byte
			if (*x < 0xd800 || *x >= 0xe000)
				return Char(static_cast<char32_t>(*x++));
			if (x + 1 >= end)
				THROW(CharError()) << "Cannot read character, buffer overflow";
			// otherwise the stored value is a codepoint which is above 0x10000
			char32_t cp = 0;
			// if the first byte is > 0xd800 and < 0xdc00, it is either high byte of surrogate pair, or possible unpaired high surrogate
			if (*x >= 0xd800 && *x < 0xdc00) // high-end surrogate pair, or high unpaired surrogate
				cp = static_cast<unsigned>(*(x++) - 0xd800) << 10;
			// now see if there is low surrogate (this happen either after high surrogate already parsed, or if the codepoint is low surrogate alone can be the first thing we see
			if (x != end && (*x >= 0xdc00 && *x < 0xdfff)) // low end surrogate pair or low unpaired surrogate
				cp += (*(x++) - 0xdc00);
			// don't forget to increment the codepoint
			return Char{static_cast<char32_t>(cp + 0x10000)};
		}

		/** Returns the UTF8 encoded character at the given address. 
		 */
		static Char FromUTF8(char const *& i, char const * end) {
			if (i >= end)
				THROW(CharError()) << "Cannot read character, buffer overflow";
			unsigned char const * u = pointer_cast<unsigned char const *>(i);
			if (*u < 0x80) {
				++i;
			    return Char{*u};
			} else if (*u < 0xe0) {
				if (i + 2 >= end)
    				THROW(CharError()) << "Cannot read character, buffer overflow";
				i += 2;
			    return Char{u[0], u[1]};
			} else if (*u < 0xf0) {
				if (i + 3 >= end)
    				THROW(CharError()) << "Cannot read character, buffer overflow";
				i += 3;
			    return Char{u[0], u[1], u[2]};
			} else {
				if (i + 4 >= end)
    				THROW(CharError()) << "Cannot read character, buffer overflow";
				i += 4;
			    return Char{u[0], u[1], u[2], u[3]};
			}
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

		char const* toCharPtr() const {
			return reinterpret_cast<char const*>(&bytes_);
		}

		/** Returns number of columns in monospace font (such as a terminal) the given character should occupy. 

		    On linux, the funtion wcwidth should do, however this does not exist on Windows and to make sure that applications behave the same on all platforms, own decission is implemented. 
		 */
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

        static int ColumnWidth(Char const & c) {
            return ColumnWidth(c.codepoint());
        }

        // TODO check for non-printable non-ASCII utf characters too
        static bool IsPrintable(char32_t c) {
            return c >= ' ' && c != 127;
        }

		/** Returns true if the given character is whitespace
		 
		    TODO only works on ASCII characters for now, extra UTF whitespace characters should be added.
		 */
		static bool IsWhitespace(Char const & what) {
			switch (what.bytes_[0]) {
				case '\t':
				case '\r':
				case '\n':
				case ' ':
				    return true;
				default:
				    return false;
			}
		}

		/** Returns true if the given character is end of line. 
		 
		    TODO only works on ASCII characters for now, extra UTF end of line codepoints should be added.
		 */
		static bool IsLineEnd(Char const & what) {
			switch (what.bytes_[0]) {
				case '\n':
				    return true;
				default:
				    return false;
			}
		}

		/** Returns true if the given character is word separator. 
		 
		    TODO only works on ASCII characters for now, extra UTF word separators should be added.
		 */
		static bool IsWordSeparator(Char const & what) {
			switch (what.bytes_[0]) {
				case '\t':
				case '\r':
				case '\n':
				case ' ':
				case ',':
				case '.':
				case ';':
				case '!':
				case '?':
				    return true;
				default:
				    return false;
			}
		}

        static bool IsDecimalDigit(char x) {
            return (x >= '0' && x <= '9');
        }

        static bool IsDecimalDigit(char x, unsigned & value) {
            if (x >= '0' && x <= '9') {
                value = x - '0';
                return true;
            } else {
                return false;
            }
        }

        static bool IsHexadecimalDigit(char x) {
            return (x >= '0' && x <= '9') || (x >= 'a' && x <= 'f') || (x >= 'A' && x <= 'F');
        }


        static bool IsHexadecimalDigit(char x, unsigned & value) {
            if (x <= '9' && x >= '0')
                value = (x - '0');
            else if (x <= 'f' && x >= 'a')
                value = (x - 'a' + 10);
            else if (x <= 'F' && x >= 'A')
                value = (x - 'A' + 10);
            else
                return false;
            return true;
        }

        static unsigned ParseHexadecimalDigit(char x) {
            if (x <= '9' && x >= '0')
                return (x - '0');
            else if (x <= 'f' && x >= 'a')
                return (x - 'a' + 10);
            else if (x <= 'F' && x >= 'A')
                return (x - 'A' + 10);
            else
                THROW(IOError()) << "Hexadecimal digit expected, but " << x << " found";
        }

        static char ToHexadecimalDigit(unsigned value) {
            ASSERT(value < 16);
            if (value < 10)
                return '0' + static_cast<char>(value);
            else 
                return 'a' - 10 + static_cast<char>(value);
        }

	private:

	    Char(unsigned char first, unsigned char second = 0, unsigned char third = 0, unsigned char fourth = 0) {
			bytes_[0] = first;
			bytes_[1] = second;
			bytes_[2] = third;
			bytes_[3] = fourth;
		}

		friend std::ostream& operator << (std::ostream& s, Char const & c) {
			s.write(reinterpret_cast<char const *>(&c.bytes_), c.size());
			return s;
		}

        /** Prepends the character to UTF8 string. 
         */
        friend std::string operator + (Char const & c, std::string const & str) {
            return STR(c << str);
        }

// macOS does not really support the C++ standard wrt char16_t. Fportunately we only need it on Windows for now
#ifndef ARCH_MACOS
		friend utf16_stringstream & operator << (utf16_stringstream & s, Char const & c) {
			unsigned cp = c.codepoint();
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
			return s;
		}
#endif

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
	}; // Char

	inline bool IsDecimalDigit(char32_t what) {
        return (what >= '0') && (what <= '9');
	}

    inline bool IsASCIILetter(char32_t what) {
        return (what >= 'a' && what <= 'z') || (what >= 'A' && what <= 'Z');
    }

	/** Determines if given character is a hexadecimal digit. 
	 
	    Numbers and letters `a`-`f` and `A`-`F` are recognized as hexadecimal digits. 
	 */
	inline bool IsHexadecimalDigit(char32_t what) {
		return ((what >= '0') && (what <= '9')) || ((what >= 'a') && (what <= 'f')) || ((what >= 'A') && (what <= 'F'));
	}

	/** Determines whether given character whitespace or not. 

	    New line, carriage return, space and tab are considered whitespace characters. 
	 */
	inline bool IsWhitespace(char32_t what) {
		return what == ' ' || what == '\t' || what == '\r' || what == '\n';
	}

    /** Returns true if the given character is a word separator. 
     */
    inline bool IsWordSeparator(char32_t c) {
        switch (c) {
            case ' ':
            case '\t':
            case '\n':
            case '\r':
            case '.':
            case ',':
            case ':':
            case ';':
            case '?':
            case '!':
            case '"':
            case '\'':
            case '/':
            case '(':
            case ')':
            case '[':
            case ']':
            case '{':
            case '}':
            case '|':
            case '*':
            case '~':
            case '<':
            case '>':
            case '`':
                return true;
            default:
                return false;
        }
    }

	/** Converts given character containing a decimal digit to its value.
	 */
	inline unsigned DecCharToNumber(char what) {
		ASSERT((what >= '0') && (what <= '9')) << "Not a decimal number: " << what << " (ASCII: " << static_cast<unsigned>(what) << ")";
		return what - '0';
	}

	/** Converts given character containing hexadecimal digit (upper and lower case variants of a-f are supported) to its value.
	 */
	inline unsigned HexCharToNumber(char what) {
		ASSERT(((what >= '0') && (what <= '9')) || ((what >= 'A') && (what <= 'F')) || ((what >= 'a') && (what <= 'f'))) << "Not a hexadecimal number: " << what << " (ASCII: " << static_cast<unsigned>(what) << ")";
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

HELPERS_NAMESPACE_END