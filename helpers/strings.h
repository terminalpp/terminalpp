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
		ASSERT(InRangeInclusive(what, '0', '9') || InRangeInclusive(what, 'A', 'F') || InRangeInclusive(what, 'a', 'f')) << "Not a hexadecimal number: " << what << " (ASCII: " << static_cast<unsigned>(what) << ")";
		if (what <= '9')
			return what - '0';
		if (what <= 'F')
			return what - 'A' + 10;
		return
			what - 'a' + 10;
	}

} // namespace helpers