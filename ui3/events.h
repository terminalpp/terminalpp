#pragma once

#include "helpers/char.h"
#include "helpers/events.h"

#include "inputs.h"
#include "geometry.h"

namespace ui3 {

    class Widget;

    class StoppableEventPayload {
    public:
        void stop() {
            active_ = false;
        }

        bool active() const {
            return active_;
        }

    protected:
        StoppableEventPayload():
            active_{true} {
        }
    private:
        bool active_;
    }; // StoppableEventPayload

    template<typename P, typename T = Widget>
    using Event = ::Event<P, T, StoppableEventPayload>;

	class MouseButtonEventPayload {
	public:
        Point coords;
		MouseButton button;
		Key modifiers;
	};

	class MouseWheelEventPayload {
	public:
        Point coords;
		int by;
		Key modifiers;
	};

	class MouseMoveEventPayload {
	public:
        Point coords;
		Key modifiers;
	};

    using VoidEvent = Event<void>;

    using KeyEvent = Event<Key>;
    using KeyCharEvent = Event<Char>;

    using MouseButtonEvent = Event<MouseButtonEventPayload>;
    using MouseWheelEvent = Event<MouseWheelEventPayload>;
    using MouseMoveEvent = Event<MouseMoveEventPayload>;

} // namespace ui