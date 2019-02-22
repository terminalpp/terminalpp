#pragma once

#include <type_traits>
#include <functional>
#include <vector>

#include "helpers.h"
#include "events.h"
namespace helpers {

	/** Base class for upgraded objects.

	    All upgraded objects have virtual destructors and are optimized for holding and triggering events. 
	 */
	class Object {
	public:
		/** Virtual destructor for the objects. 
		 */
		virtual ~Object() {
		}

	protected:

		/** Triggers given event with itself as a sender. 

		    SFINAE makes sure that this method is used only for events with void payloads. 
		 */
		template<typename EVENT>
		void trigger(EVENT & e) {
			static_assert(std::is_same<EVENT::Payload::Sender, Object >::value, "Only events with sender being Object should be used");
			EVENT::Payload p(this);
			e.trigger(p);
		}

		/** Triggers given event with itself as a sender.

			SFINAE makes sure that this method is used only for events with void payloads.
		 */
		template<typename EVENT>
		void trigger(EVENT & e, typename EVENT::Payload::Payload const & payload) {
			static_assert(std::is_same<EVENT::Payload::Sender, Object >::value, "Only events with sender being Object should be used");
			EVENT::Payload p(this, payload);
			e.trigger(p);
		}
	};

} // namespace helpers