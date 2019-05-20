#pragma once

#include <ostream>

namespace vterm {

	enum class MouseButton {
		Left,
		Right,
		Wheel
	};

	inline std::ostream& operator << (std::ostream& s, MouseButton const& b) {
		switch (b) {
		case MouseButton::Left:
			s << "Left button";
			break;
		case MouseButton::Right:
			s << "Right button";
			break;
		case MouseButton::Wheel:
			s << "Wheel button";
			break;
		default:
			UNREACHABLE;
		}
		return s;
	}

} // namespace vterm