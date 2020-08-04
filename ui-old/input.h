#pragma once

#include "helpers/helpers.h"


namespace ui {

    /** Mouse buttons are organized as a bitset. 
     */
	enum class MouseButton {
		Left = 1,
		Right = 2,
		Wheel = 4
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

	/** Describes a key for the key up and down events. 

	    Any representation is as good as any other, so we are reusing the Win32 virtual key mappings where appropriate for the key codes. This has the benefit of printable US keyboard keys being identical to their ASCII codes. 

	 */
	class Key {
	public:

#define KEY(NAME, CODE) static constexpr unsigned NAME = CODE;
#include "keys.inc.h"

		/* Modifiers 
		 */
		static constexpr unsigned Shift = 1 << 16;
		static constexpr unsigned Ctrl = 1 << 17;
		static constexpr unsigned Alt = 1 << 18;
		static constexpr unsigned Win = 1 << 19;

		static constexpr unsigned Invalid = 0;


		static bool IsValidCode(unsigned c);

		Key() :
			raw_(Invalid) {
		}

		Key(unsigned code, unsigned modifiers = 0) :
			raw_(code | modifiers) {
			ASSERT((code & 0xffff0000) == 0);
			ASSERT((modifiers & 0xfff0ffff) == 0);
		}

		unsigned code() const {
			return raw_ & 0xffff;
		}

		unsigned modifiers() const {
			return raw_ & 0x000f0000;
		}

		bool operator == (unsigned other) const {
			return raw_ == other;
		}

		bool operator != (unsigned other) const {
			return raw_ != other;
		}

		bool operator & (unsigned modifier) const {
			ASSERT((modifier & 0xfff0ffff) == 0);
			return raw_ & modifier;
		}

		Key operator + (unsigned modifier) const {
			ASSERT((modifier & 0xfff0ffff) == 0);
			Key result;
			result.raw_ = raw_ | modifier;
			return result;
		}

		bool operator == (Key const & other) const {
			return raw_ == other.raw_;
		}

		bool operator != (Key const & other) const {
			return raw_ != other.raw_;
		}

	private:

	    friend struct std::hash<Key>;

	    unsigned raw_;

	};

	inline std::ostream& operator << (std::ostream& s, Key k) {
		if (k & Key::Shift)
			s << "S-";
		if (k & Key::Ctrl)
			s << "C-";
		if (k & Key::Alt)
			s << "A-";
		if (k & Key::Win)
			s << "W-";
		switch (k.key()) {
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

} // namespace ui

namespace std {

	template<>
	struct hash<ui::Key> {
		size_t operator () (ui::Key const & key) const {
			return std::hash<unsigned>{}(key.raw_);
		}
	}; // std::hash<ui::Key>

} // namespace std