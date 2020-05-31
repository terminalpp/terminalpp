#pragma once

#include <thread>

#include "helpers/char.h"
#include "helpers/events.h"

#include "input.h"
#include "geometry.h"


/** TODO this will be represented by a templated monstrosity that would lock based on the renderer, i.e. one thread per renderer. 
 */
#ifndef NDEBUG
#define UI_THREAD_CHECK if (ui::UIThreadChecker_::ThreadId() != std::this_thread::get_id()) THROW(Exception()) << "Only UI thread is allowed to execute at this point"
#else
#define UI_THREAD_CHECK
#endif



namespace ui {

    class Widget;

    class EventPayloadBase {
    public:
        /** Prevents the default behavior for the event. 
         */
        void stop() {
            active_ = false;
        }

        bool active() const {
            return active_;
        }

        /** Enables or disables the event propagation to the parent widget for events that support it. 
         */
        void propagateToParent(bool value = true) {
            propagateToParent_ = value;
        } 

        /** Returns whether the event was accepted or not. 
         */
        bool shouldPropagateToParent() const {
            return propagateToParent_;
        }

    protected:
        EventPayloadBase():
            active_{true},
            propagateToParent_{false} {
        }
    private:
        bool active_;
        bool propagateToParent_;
    }; 

    template<typename P, typename T = Widget>
    using UIEvent = ::Event<P, T, EventPayloadBase>;

	class MouseButtonEvent {
	public:
        Point coords;
		MouseButton button;
		Key modifiers;
	};

	class MouseWheelEvent {
	public:
        Point coords;
		int by;
		Key modifiers;
	};

	class MouseMoveEvent {
	public:
        Point coords;
		Key modifiers;
	};



#ifndef NDEBUG

    /** A simple RAII debug check that all UI operations are always done in a single thread. 
     */
    class UIThreadChecker_ {
    public:
        static std::thread::id ThreadId();
    };

#endif


} // namespace ui