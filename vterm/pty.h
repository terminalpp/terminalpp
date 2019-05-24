#pragma once

#include <cstring>

#include "helpers/object.h"

#include "terminal.h"

namespace vterm {

	/** Pseudoterminal base api which specifies blocking send and receive methods. 
	 */
	class PTY : public helpers::Object {
	public:
		typedef helpers::EventPayload<int, helpers::Object> TerminatedEvent;

		/** This event should be raised when the underlying process is terminated. 
		 */
		helpers::Event<TerminatedEvent> onTerminated;

		/** Returns true if the process has terminated. 
		 */
		bool terminated() const {
			return terminated_;
		}

		/** Returns the exit status of the process if it has terminated. 

		    Should not be called if the process has not terminated yet. 
		 */
		int exitStatus() const {
			ASSERT(terminated_) << "Undefined value for still running processes";
			return exitStatus_;
		}

		/** Terminates the underlying process. 

		    Terminating the process explicitly via the terminate() method should not invoke the onTerminated event. The actual termination is left for the children to implement.
		 */
		virtual void terminate() {
			terminated_ = true;
		}

		virtual ~PTY() {
		}


	protected:
		friend class Terminal::PTYBackend;

		PTY() :
			terminated_(false),
			exitStatus_(0) {
		}

		/** Called when data should be sent to the target process. 

		    Sends the buffer of given size to the target process. Returns the number of bytes sent, which is identical to the size of the buffer given unless there was a failure. 
		 */
		virtual size_t sendData(char const * buffer, size_t size) = 0;

		/** Waits for the target process to send data and populates the given buffer. 

		    Up to availableSize bytes can be read at once, but the actual number of bytes received is to be returned by the function. 
		 */
		virtual size_t receiveData(char * buffer, size_t availableSize) = 0;

		/** Notifies the underlying terminal process that the terminal size has changed to given values. 
		 */
		virtual void resize(unsigned cols, unsigned rows) = 0;

		volatile bool terminated_;
		int exitStatus_;


	}; // vterm::PTY

} // namespace vterm