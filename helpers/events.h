#pragma once

#include <functional>
#include <vector>

#include "helpers.h"

namespace helpers {

	/** Encapsulation around event handler function or method.
	 */
	template<typename PAYLOAD>
	class EventHandler {
	public:

		/** Creates event handler for given function.
		 */
		EventHandler(void(*f)(PAYLOAD &)) :
			object_(nullptr),
			function_(f),
			f_(f) {
			ASSERT(f != nullptr) << "Cannot create function handler for nullptr function";
		}

		/** Creates event handler for the specified object and its method.
		 */
		template<typename C>
		EventHandler(C * object, void (C:: * method)(PAYLOAD &)) :
			object_(object),
			function_(reinterpret_cast<void*>(method)) {
			f_ = std::bind(method, object, std::placeholders::_1);
			ASSERT(object != nullptr) << "Cannot create method handler for nullptr object";
		}

		/** Calls the stored handler.
		 */
		void operator() (PAYLOAD & payload) {
			f_(payload);
		}

		/** Compares two handlers.

			Handler functions are identical if they point to the same function. Handler methods are identical if they point to the same method of the same object.
		 */
		bool operator == (EventHandler<PAYLOAD> const & other) const {
			return function_ == other.function_ &&  object_ == other.object_;
		}

		bool operator != (EventHandler<PAYLOAD> const & other) const {
			return function_ != other.function_ || object_ != other.object_;
		}

	private:
		/** If the handler is a method, holds the handler object.
		 */
		void * object_;
		/** Pointer to the handler function, or handler method.
		 */
		void * function_;
		/** The function object to call.
		 */
		std::function<void(PAYLOAD &)> f_;
	};


	template<typename PAYLOAD>
	class Event {
	public:

		typedef PAYLOAD Payload;

		Event & operator += (EventHandler<PAYLOAD> const & handler) {
			handlers_.emplace_back(handler);
			return *this;
		}

		Event & operator -= (EventHandler<PAYLOAD> const & handler) {
			for (auto i = handlers_.begin(), e = handlers_.end(); i != e; ++i) {
				if (*i == handler) {
					handlers_.erase(i);
					return *this;
				}
			}
			UNREACHABLE;
		}

		/** NOTE Idellhy this method would only be accessible by the sender classes, however in C++ it is not possible to friend template argument, the reasons of which are unknown to me.
		 */
		void trigger(PAYLOAD & payload) {
			for (auto & h : handlers_) {
				if (!payload.doDispatch_)
					break;
				h(payload);
			}
		}

	private:

		std::vector<EventHandler<PAYLOAD>> handlers_;

	};

	class Object;

	template<typename PAYLOAD, typename SENDER>
	class EventPayload {
	public:
		typedef SENDER Sender;
		typedef PAYLOAD Payload;

		SENDER * const sender;

		EventPayload(SENDER * sender, PAYLOAD const & payload) :
			sender(sender),
			payload_(payload),
			doDispatch_(true) {
		}

		/** Stops dispatching the current event.

			When an event handler calls the stopDispatch() method, no further dispatch events will be generated.
		 */
		void stopDispatch() {
			doDispatch_ = false;
		}

		/** Dereferencing event payload returns the payload object itself.
		 */
		PAYLOAD & operator * () {
			return payload_;
		}

		PAYLOAD * operator -> () {
			return &payload_;
		}

	private:
		friend class Event<EventPayload<PAYLOAD, SENDER>>;
		PAYLOAD payload_;
		bool doDispatch_;
	};

	template<typename SENDER>
	class EventPayload<void, SENDER> {
	public:
		typedef SENDER Sender;

		SENDER * const sender;

		EventPayload(SENDER * sender) :
			sender(sender),
			doDispatch_(true) {
		}
		/** Stops dispatching the current event.

			When an event handler calls the stopDispatch() method, no further dispatch events will be generated.
		 */
		void stopDispatch() {
			doDispatch_ = false;
		}
	private:
		friend class Event<EventPayload<void, SENDER>>;
		bool doDispatch_;
	};


} // namespace helpers