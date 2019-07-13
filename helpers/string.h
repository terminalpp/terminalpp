#pragma once

#include "helpers.h"

#include "char.h"

namespace helpers {

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
	  */
	inline std::string UTF16toUTF8(char16_t const* str) {
		std::stringstream result;
		while (*str != 0) {
			result << Char::FromUTF16(str, str + 2); // null terminated so we assume there is enough space
		}
		return result.str();
	}

#ifdef _WIN64
	/** On windows, the UTF16 can be expressed in wchar_t array as well because of the same size. 
	 */
	inline std::string UTF16toUTF8(wchar_t const* str) {
		static_assert(sizeof(wchar_t) == sizeof(char16_t), "wchar_t on windows assumed to be same size as char16_t");
		return UTF16toUTF8(reinterpret_cast<char16_t const*>(str));
	}
#endif

// macOS does not really support the C++ standard wrt char16_t. Fportunately we only need it on Windows for now
#ifndef __APPLE__
	inline std::u16string UTF8toUTF16(char const* str) {
		utf16_stringstream result;
		while (*str != 0) {
			Char const * c = Char::At(str, str + 4); // null terminated, so we assume there is enough space
			// just stop if the character looks invalid
			if (c == nullptr)
				break;
			c->toUTF16(result);
		}
		return result.str();
	}
#endif

} // namespace helpers