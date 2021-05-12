#pragma once

#include <functional>
#include <vector>

#include "helpers.h"

HELPERS_NAMESPACE_BEGIN

    /** The default payload base. 
     */
    class DefaultPayloadBase {
    };

	/** 
	 */
    template<typename PAYLOAD, typename SENDER, typename PAYLOAD_BASE = DefaultPayloadBase>
	class Event {
	public:
		typedef SENDER sender_type;

	    class Payload : public PAYLOAD_BASE {
		public:
		    typedef PAYLOAD payload_type;
    		typedef SENDER sender_type;

            explicit Payload(PAYLOAD const & payload):
                payload_{payload},
                sender_{nullptr} {
            } 

            template<typename... ARGS>
            explicit Payload(ARGS... args):
                payload_{PAYLOAD{args...}},
                sender_{nullptr} {
            }

		    SENDER * sender() const {
				return sender_;
			}

            void updateSender(SENDER * value) {
                sender_ = value;
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

			PAYLOAD payload_;
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

        void operator() (Payload & payload, SENDER * sender) {
            if (handler_) {
                payload.updateSender(sender);
                handler_(payload);
            }
        }

        void operator() (Payload && payload, SENDER * sender) {
            if (handler_) {
                payload.updateSender(sender);
                handler_(payload);
            }
        }

	private:
	    std::function<void(Payload &)> handler_;

	}; // Event<PAYLOAD, SENDER, PAYLOAD_BASE>

    template<typename SENDER, typename PAYLOAD_BASE>
	class Event<void, SENDER, PAYLOAD_BASE> {
	public:
		typedef SENDER sender_type;

	    class Payload : public PAYLOAD_BASE {
		public:
		    typedef void payload_type;
    		typedef SENDER sender_type;

            Payload() = default;

		    SENDER * sender() const {
				return sender_;
			}

            void updateSender(SENDER * value) {
                sender_ = value;
            }

		private:

			explicit Payload(SENDER * sender):
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

        void operator() (Payload & payload, SENDER * sender) {
            if (handler_) {
                payload.updateSender(sender);
                handler_(payload);
            }
        }

        void operator() (Payload && payload, SENDER * sender) {
            if (handler_) {
                payload.updateSender(sender);
                handler_(payload);
            }
        }


	private:
	    std::function<void(Payload &)> handler_;

	}; // Event<void, SENDER, PAYLOAD_BASE>

HELPERS_NAMESPACE_END