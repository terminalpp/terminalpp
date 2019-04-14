#include "key.h"

namespace vterm {

	bool Key::IsValidCode(unsigned c) {
		switch (c) {
#define KEY(NAME, CODE) case CODE:
#include "keys.inc.h"
				return true;
			default:
				return false;
		}
	}

	std::ostream& operator << (std::ostream& s, Key k) {
		if (k | Key::Shift)
			s << "S-";
		if (k | Key::Ctrl)
			s << "C-";
		if (k | Key::Alt)
			s << "A-";
		if (k | Key::Meta)
			s << "M-";
		switch (k.code()) {
		case Key::Invalid:
			s << "Invalid";
			break;
#define KEY(NAME, CODE) case CODE: s << #NAME; break;
#include "keys.inc.h"
		default:
			s << "Unknown Key";
			break;
		}
		return s;
	}


} // namespace vterm