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

		 TODO proper conversion
	  */
	inline std::string UTF16toUTF8(wchar_t const* str) {
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

	inline std::wstring UTF8toUCS2(std::string const& what) {
		return UTF8toUTF16(what.c_str());
	}



} // namespace helpers