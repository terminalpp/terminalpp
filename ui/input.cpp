#include "input.h"

namespace ui2 {

	bool Key::IsValidCode(unsigned c) {
		switch (c) {
#define KEY(NAME, CODE) case CODE:
#include "keys.inc.h"
				return true;
			default:
				return false;
		}
	}

} // namespace ui