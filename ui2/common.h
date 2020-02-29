#pragma once

#include "helpers/char.h"
#include "helpers/events.h"

#include "input.h"


/** TODO this will be represented by a templated monstrosity that would lock based on the renderer, i.e. one thread per renderer. 
 */
#define UI_THREAD_CHECK



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



    class Renderer;

    /** A simple RAII debug check that all UI operations are always done in a single thread. 
     
        Note that instead of forcing a thread, which would be safer, but more obtrusive, this only checks at runtime that no threads ever cross accessing functions intended to run in the UI thread only. This means that if the client code uses multiple threads, but makes sure that they never cross accessing UI events, the checks will pass. 

     */
    class UiThreadChecker_ {
    public:
        /** Constructor. 
         
            The constructor is templated so that 
         */
        template<typename T>
        UiThreadChecker_(T * object):
            renderer_{object->getRenderer_()} {
            initialize();
        }

        ~UiThreadChecker_();

    private:

        void initialize();

        /** The renderer against which the single-threadness is tested. 
         */
        Renderer * renderer_;
    };


} // namespace ui