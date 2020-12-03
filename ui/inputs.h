#pragma once

#include "helpers/helpers.h"

namespace ui {

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

    /** Mouse cursor. 
     
        A predefined set of mouse cursors is supported for the widgets to choose. These widgets will look different on different platforms and some platforms may not support all of the options available. If a particular mouse cursor is not supported by given renderer on the platform, the nearest possible match should be used and no warnings are expected to be given. 
     */
    enum class MouseCursor {
        /** Default cursor, based on the application. 
         */
        Default,
        /** Standard mouse arrow.
         */
        Arrow,
        /** Mouse cursor for actionable items such as buttons, usually in the shape of a hand. 
         */
        Hand,
        /** Text caret cursor. 
         */
        Beam,
        /** Vertical (height) size update cursor.
         */
        VerticalSize,
        /** Horizontal (width) size update cursor.
         */
        HorizontalSize,
        /** Vertical (top/bottom) split. 
         */
        VerticalSplit,
        /** Horizontal (left/right) split.
         */
        HorizontalSplit,
        /** Cursor indicating task in progress, usually a hourglass or a spinner.
         */
        Wait,
        /** Forbidden action, ghostbusters sign. 
         */
        Forbidden,
    };

    // Keyboard

    class Key {
        friend struct std::hash<Key>;
    public:

        static Key const Invalid;
        static Key const None;

#define KEY(NAME, CODE) static Key const NAME;
#include "keys.inc.h"

        static Key const Shift;
        static Key const Ctrl;
        static Key const Alt;
        static Key const Win;

        static Key FromCode(unsigned code);

        Key():
            raw_{InvalidCode} {
        }

        Key key() const {
            return Key{raw_ & 0xffff};
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

        /** Returns true if the key consists only of modifier key's status (i.e. no key is actually pressed as partof the event, including the modifier keys) 
         */
        bool isModifier() const {
            return raw_ != 0 && code() == 0;
        }

        /** Returns true if the reported key is pressed is a modifier. 
         */
        bool isModifierKey() const {
            return key() == Key::ShiftKey || key() == Key::CtrlKey || key() == Key::AltKey || key() == Key::WinKey;
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
            ASSERT(modifier.code() == 0 || code() == 0) << "Only modifiers can be added to a key";
            return Key{raw_ | (modifier.modifiers().raw_)};
        }

        Key & operator += (Key const & modifier) {
            ASSERT(modifier.code() == 0 || code() == 0) << "Only modifiers can be added to a key";
            raw_ |= modifier.modifiers().raw_;
            return *this;
        }

        Key operator - (Key const & modifier) const {
            ASSERT(modifier.code() == 0 || code() == 0) << "Only modifiers can be removed from a key";
            return Key{raw_ & ~(modifier.modifiers().raw_)};
        }

        Key & operator -= (Key const & modifier) {
            ASSERT(modifier.code() == 0 || code() == 0) << "Only modifiers can be removed from a key";
            raw_ &= ~modifier.modifiers().raw_;
            return *this;
        }


    private:
        static constexpr unsigned InvalidCode = 0;

        explicit Key(unsigned raw):
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
            switch (k.raw_ & 0xffff) {
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

namespace std {

    template<>
    struct hash<ui::Key> {
        size_t operator () (ui::Key const & x) const {
            return std::hash<unsigned>()(x.raw_);
        }
    };

} // namespace std