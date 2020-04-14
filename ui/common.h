#pragma once

#include <thread>

#include "helpers/char.h"
#include "helpers/events.h"

#include "input.h"
#include "geometry.h"


/** TODO this will be represented by a templated monstrosity that would lock based on the renderer, i.e. one thread per renderer. 
 */
#ifndef NDEBUG
#define UI_THREAD_CHECK if (ui::UIThreadChecker_::ThreadId() != std::this_thread::get_id()) THROW(helpers::Exception()) << "Only UI thread is allowed to execute at this point"
#else
#define UI_THREAD_CHECK
#endif



namespace ui {

    class Widget;

    using Char = helpers::Char;

    // TODO rename this to PreventablePayloadBase ?
    class CancellablePayloadBase {
    public:
        /** Prevents the default behavior for the event. 
         */
        void stop() {
            active_ = false;
        }

        bool active() const {
            return active_;
        }

    protected:
        CancellablePayloadBase():
            active_{true} {
        }
    private:
        bool active_;
    }; 

    template<typename P, typename T = Widget>
    using Event = helpers::Event<P, T, CancellablePayloadBase>;

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