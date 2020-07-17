#pragma once

#include "helpers/helpers.h"

namespace ui3 {

    // Mouse

    /** Mouse button identifier.

        Organized as a bitset. 
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

    // Keyboard

    class Key {
        friend struct std::hash<Key>;
    public:

        static constexpr unsigned InvalidCode = 0;

        static Key const Invalid;

#define KEY(NAME, CODE) static Key const NAME;
#include "keys.inc.h"

        static Key const Shift;
        static Key const Ctrl;
        static Key const Alt;
        static Key const Win;

        Key():
            raw_{InvalidCode} {
        }

        unsigned code() const {
            return raw_ & 0xffff;
        }

        Key modifiers() const {
            return Key{raw_ & 0xffff0000};
        }

        Key stripModifiers() const {
            return Key{raw_ & 0xffff};
        }

		bool operator == (Key const & other) const {
			return raw_ == other.raw_;
		}

		bool operator != (Key const & other) const {
			return raw_ != other.raw_;
		}

        bool operator & (Key const & modifier) const {
            ASSERT(modifier.code() == 0 && modifier.raw_ != 0) << "Only modifiers can be checked";
            return raw_ & modifier.raw_;
        }

        Key operator + (Key const & modifier) const {
            ASSERT(modifier.code() == 0) << "Only modifiers can be added to a key";
            return Key{raw_ | (modifier.modifiers().raw_)};
        }

        Key & operator += (Key const & modifier) {
            ASSERT(modifier.code() == 0) << "Only modifiers can be added to a key";
            raw_ |= modifier.modifiers().raw_;
            return *this;
        }

    private:

        Key(unsigned raw):
            raw_{raw} {
        }

        friend std::ostream & operator << (std::ostream & s, Key const & k) {
            if (k & Key::Shift)
                s << "S-";
            if (k & Key::Ctrl)
                s << "C-";
            if (k & Key::Alt)
                s << "A-";
            if (k & Key::Win)
                s << "W-";
            switch (k.code()) {
            case Key::InvalidCode:
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

        unsigned raw_;
    }; // ui::Key



} // namespace ui