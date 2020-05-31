#pragma once

#include "helpers.h"

HELPERS_NAMESPACE_BEGIN

	inline unsigned Base64DecodeCharacter(char what) {
		if (what >= 'A' && what <= 'Z')
			return what - 'A';
		if (what >= 'a' && what <= 'z')
			return what - 'a' + 26;
		if (what >= '0' && what <= '9')
			return what - '0' + 52;
		if (what == '+')
			return 62;
		if (what == '/')
			return 63;
		if (what == '=')
			return 0;
		ASSERT(false) << "Invalid base64 character " << static_cast<unsigned>(what);
		UNREACHABLE;
	}

	/** Decodes given base64 input stream into a string. 

	    Supports either padding with the `=` characters, or variable length input
	 */
	inline std::string Base64Decode(char* start, char* end) {
		std::stringstream result;
		// do all but the last triplet
		while (start + 4 < end) {
			unsigned x =
				(Base64DecodeCharacter(start[0]) << 18) +
				(Base64DecodeCharacter(start[1]) << 12) +
				(Base64DecodeCharacter(start[2]) << 6) +
				(Base64DecodeCharacter(start[3]));
			result << reinterpret_cast<char const*>(&x)[2];
			result << reinterpret_cast<char const*>(&x)[1];
			result << reinterpret_cast<char const*>(&x)[0];
			start += 4;
		}
		// there must be at least 2 bytes available
		if (start + 2 <= end) {
			unsigned x =
				(Base64DecodeCharacter(start[0]) << 18) +
				(Base64DecodeCharacter(start[1]) << 12);
			result << reinterpret_cast<char const*>(&x)[2];
			start += 2;
			if (start != end && *start != '=') {
				x += (Base64DecodeCharacter(*start) << 6);
				result << reinterpret_cast<char const*>(&x)[1];
				if (++start != end && *start != '=') {
					x += Base64DecodeCharacter(*start);
					result << reinterpret_cast<char const*>(&x)[0];
				}
			}
		}
		return result.str();
	}



HELPERS_NAMESPACE_END