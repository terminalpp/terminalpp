#pragma once

#include <functional>
#include <vector>

#include "helpers.h"


namespace helpers {

	/** Encapsulation around event handler function or method.

	    If the handler is a function, the function pointer is kept inside with the target object being set to nullptr, in case of methods, this is more complicated as a method pointer does not have fixed (or even defined) size. What we therefore do is use the method pointer as a template argument of a wrapper function, whose pointer we store together with the object to be called. When triggered, the object and payload are passed to the wrapper function.

		TODO Creating a handler is now a mouthful, it would be really nice if this can be simplified using template deductions (notably the method case), but I am not even sure if such magic is possible in C++. 
	 */
	template<typename PAYLOAD>
	class EventHandler {
	public:

		/** Event handlers can be compared for equality, i.e. whether they point to the same target function, or method & object. 
		 */
		bool operator == (EventHandler<PAYLOAD> const & other) const {
			return (f_ == other.f_) && (object_ == other.object_);
		}

		bool operator != (EventHandler<PAYLOAD> const & other) const {
			return (f_ != other.f_) || (object_ != other.object_);
		}

		/** Triggers the handler. 
		 */
		void operator() (PAYLOAD & payload) {
			if (object_ == nullptr)
				(*f_)(payload);
			else
				(*w_)(object_, payload);
		}

	private:
		typedef void (*FunctionPtr) (PAYLOAD &); 
		typedef void (* WrapperPtr)(void *, PAYLOAD &);

		template<typename P>
		friend EventHandler<P> CreateHandler(void (*fptr)(P &));

		template<typename P, typename C, void(C::*M)(P &)>
		friend EventHandler<P> CreateHandler(C* target);

		EventHandler(FunctionPtr f):
			object_(nullptr),
			f_(f) {
			static_assert(sizeof(FunctionPtr) == sizeof(WrapperPtr), "Function and wrapper are of the same size");
		}

		EventHandler(void * object, WrapperPtr w):
			object_(object),
			w_(w) {
			static_assert(sizeof(FunctionPtr) == sizeof(WrapperPtr), "Function and wrapper are of the same size");
			ASSERT(object != nullptr) << "When calling a method, target object must not be null";
		}

		template<typename C, void(C::*M)(PAYLOAD &)>
		static void Wrapper(void * object, PAYLOAD & p) {
			C * target = static_cast<C*>(object);
			(*target.*M)(p);
		} 

		void * object_;
		
		union {
			FunctionPtr f_;
			WrapperPtr w_;
		};
	}; 

	/** Creates event handler from given function pointer. 
	 
	    The function pointer is the argument to the method. If template deduction does not work, the payload type must be specified, i.e. :

		    EventHandler<Payload> h = CreateHandler<Payload>([](Payload & e){ ... });
	 */
	template<typename PAYLOAD>
	inline EventHandler<PAYLOAD> CreateHandler(void (*fptr)(PAYLOAD &)) {
		return EventHandler<PAYLOAD>(fptr);
	}

	/** Creates event handler from given method pointer. 
	 
	    Note that the method pointer is template argument, while the target object is argument to the function. Since I did not figure out how to force C++ to deduce the template argument, creation of a handler from own method is rather verbose, i.e. if class Foo has instance foo and method bar, and the event payload is called Payload, we must do the following:

		    EventHandler<Payload> h = CreateHandler<Payload, Foo, &Foo::bar>(&foo);

	 */
	template<typename PAYLOAD, typename C, void(C::*M)(PAYLOAD &)>
	inline EventHandler<PAYLOAD> CreateHandler(C * target) {
		return EventHandler<PAYLOAD>(static_cast<void*>(target), & EventHandler<PAYLOAD>::template Wrapper<C,M>);
	} 

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

		template<typename T = SENDER>
		T * sender() const {
			return dynamic_cast<T*>(sender_);
		}

		EventPayload(SENDER * sender, PAYLOAD const & payload) :
			sender_(sender),
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
		SENDER * const sender_;
		PAYLOAD payload_;
		bool doDispatch_;
	};

	template<typename SENDER>
	class EventPayload<void, SENDER> {
	public:
		typedef SENDER Sender;

		template<typename T = SENDER>
		T * sender() const {
			return dynamic_cast<T>(sender_);
		}

		EventPayload(SENDER * sender) :
			sender_(sender),
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
		SENDER * const sender_;
		bool doDispatch_;
	};

} // namespace helpers