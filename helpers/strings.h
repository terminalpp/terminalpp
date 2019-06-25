#pragma once

#include "helpers.h"

namespace helpers {

	inline bool IsDecimalDigit(char what) {
		return InRangeInclusive(what, '0', '9');
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
		if (!(InRangeInclusive(what, '0', '9') || InRangeInclusive(what, 'A', 'F') || InRangeInclusive(what, 'a', 'f')))
			ASSERT(false);
		ASSERT(InRangeInclusive(what, '0', '9') || InRangeInclusive(what, 'A', 'F') || InRangeInclusive(what, 'a', 'f')) << "Not a hexadecimal number: " << what << " (ASCII: " << static_cast<unsigned>(what) << ")";
		if (what <= '9')
			return what - '0';
		if (what <= 'F')
			return what - 'A' + 10;
		return
			what - 'a' + 10;
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

	inline std::string ConvertNonPrintableCharacters(std::string const& from) {
		std::stringstream str;
		for (char c : from) {
			switch (c) {
			case '\a': // BEL, 0x07
				str << "\\a";
				break;
			case '\b': // backspace, 0x08
				str << "\\b";
				break;
			case '\t': // tab, 0x09
				str << "\\t";
				break;
			case '\n': // new line, 0x0a
				str << "\\n";
				break;
			case '\v': // vertical tab, 0x1b
				str << "\\v";
				break;
			case '\f': // form feed, 0x1c
				str << "\\f";
				break;
			case '\r': // carriage return, 0x1d
				str << "\\r";
				break;
			default:
				if (c >= 0 && c < ' ') {
					str << "\\x" << ToHexDigit(c / 16) << ToHexDigit(c % 16);
				} else {
					str << c;
				}
			}
		}
		return str.str();
	}

	/** Converts a null terminated wide string in UTF-16 encoding into an std::string encoded in UTF-8.

	    TODO proper conversion
	 */
	inline std::string UTF16toUTF8(wchar_t const * str) {
		std::stringstream result;
		while (*str != 0) {
			if (*str < 128)
				result << static_cast<char>(*str);
			++str;
		}
		return result.str();
	}

	inline std::wstring UTF8toUTF16(char const* str) {
		std::wstringstream result;
		while (*str != 0) {
			result << static_cast<wchar_t>(*str);
			++str;
		}
		return result.str();
	}

} // namespace helpers