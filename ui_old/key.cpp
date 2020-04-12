#include "key.h"

namespace ui {

	bool Key::IsValidCode(unsigned c) {
		switch (c) {
#define KEY(NAME, CODE) case CODE:
#include "keys.inc.h"
				return true;
			default:
				return false;
		}
	}

} // namespace vterm