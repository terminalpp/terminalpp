#pragma once

#include "helpers/char.h"
#include "helpers/events.h"

#include "input.h"
#include "geometry.h"


/** TODO this will be represented by a templated monstrosity that would lock based on the renderer, i.e. one thread per renderer. 
 */
#ifndef NDEBUG
#define UI_THREAD_CHECK ui::UiThreadChecker_ uiThreadChecker_{this}
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

    class Renderer;

    /** A simple RAII debug check that all UI operations are always done in a single thread. 
     
        Note that instead of forcing a thread, which would be safer, but more obtrusive, this only checks at runtime that no threads ever cross accessing functions intended to run in the UI thread only. This means that if the client code uses multiple threads, but makes sure that they never cross accessing UI events, the checks will pass. 

     */
    class UiThreadChecker_ {
    public:
        /** Constructor. 
         */
        template<typename T>
        UiThreadChecker_(T * object):
            renderer_{object->getRenderer_()} {
            if (renderer_ != nullptr)
                initialize();
        }

        ~UiThreadChecker_() noexcept(false);

    private:

        void initialize();

        /** The renderer against which the single-threadness is tested. 
         */
        Renderer * renderer_;
    };

#endif


} // namespace ui