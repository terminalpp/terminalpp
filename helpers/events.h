#pragma once

#include <functional>
#include <vector>

#include "helpers.h"


namespace helpers {

	/** 
	 */
    template<typename PAYLOAD, typename SENDER>
	class Event {
	public:
		typedef SENDER sender_type;

	    class Payload {
		public:
		    typedef PAYLOAD payload_type;
    		typedef SENDER sender_type;

		    SENDER * sender() const {
				return sender_;
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
		    friend class Event;

			Payload(PAYLOAD & payload, SENDER * sender):
			    payload_(payload),
				sender_(sender) {
			}

			PAYLOAD & payload_;
			SENDER * sender_;

		}; 

		bool attached() const {
			if (handler_)
			    return true;
			else
			    return false;
		}

		void clear() {
			handler_ = nullptr;
		}

		Event & operator = (std::function<void(Payload &)> fun) {
			handler_ = fun;
			return *this;
		}

		void setHandler(std::function<void(Payload &)> fun) {
			handler_ = fun;
		}

		template<typename C>
		void setHandler(void(C::*method)(Payload &), C * object) {
			handler_ = std::bind(method, object, std::placeholders::_1);
		}

		void operator() (SENDER * sender, PAYLOAD & payload) {
			if (handler_) {
				Payload p{payload, sender};
				handler_(p);
			}
		}

		void operator() (SENDER * sender, PAYLOAD const & payload) {
			if (handler_) {
				PAYLOAD p{payload};
				Payload pp{p, sender};
				handler_(pp);
			}
		}

		void operator() (SENDER * sender, PAYLOAD && payload) {
			if (handler_) {
				Payload p{payload, sender};
				handler_(p);
			}
		}

	private:
	    std::function<void(Payload &)> handler_;

	}; // Event<PAYLOAD, SENDER>



    template<typename SENDER>
	class Event<void, SENDER> {
	public:
		typedef SENDER sender_type;

	    class Payload {
		public:
		    typedef void payload_type;
    		typedef SENDER sender_type;

		    SENDER * sender() const {
				return sender_;
			}

		private:
		    friend class Event;

			Payload(SENDER * sender):
				sender_(sender) {
			}

			SENDER * sender_;

		}; 

		void reset() {
			handler_ = nullptr;
		}

		bool attached() const {
			if (handler_)
			    return true;
			else
			    return false;
		}

		void clear() {
			handler_ = nullptr;
		}

		Event & operator = (std::function<void(Payload &)> fun) {
			handler_ = fun;
			return *this;
		}

		void setHandler(std::function<void(Payload &)> fun) {
			handler_ = fun;
		}

		template<typename C>
		void setHandler(void(C::*method)(Payload &), C * object) {
			handler_ = std::bind(method, object, std::placeholders::_1);
		}

		void operator() (SENDER * sender) {
			if (handler_) {
				Payload p{sender};
			    handler_(p);
			}
		}

	private:
	    std::function<void(Payload &)> handler_;

	}; // Event<void, SENDER>

} // namespace helpers