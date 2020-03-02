#pragma once

#include "helpers/char.h"
#include "helpers/events.h"

#include "input.h"


/** TODO this will be represented by a templated monstrosity that would lock based on the renderer, i.e. one thread per renderer. 
 */
#ifndef NDEBUG
#define UI_THREAD_CHECK ui2::UiThreadChecker_ uiThreadChecker_{this->getRenderer_()}
#else
#define UI_THREAD_CHECK
#endif



namespace ui2 {

    class Widget;

    using Char = helpers::Char;

    class CancellablePayloadBase {
    public:
        /** Prevents the default behavior for the event. 
         */
        void preventDefault() {

        }
    }; 

    template<typename P, typename T = Widget>
    using Event = helpers::Event<P, T, CancellablePayloadBase>;

	class MouseButtonEvent {
	public:
		int x;	
		int y;
		MouseButton button;
		Key modifiers;
	};

	class MouseWheelEvent {
	public:
		int x;
		int y;
		int by;
		Key modifiers;
	};

	class MouseMoveEvent {
	public:
		int x;
		int y;
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

        ~UiThreadChecker_();

    private:

        void initialize();

        /** The renderer against which the single-threadness is tested. 
         */
        Renderer * renderer_;
    };

#endif


} // namespace ui