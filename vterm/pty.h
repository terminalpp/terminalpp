#pragma once

#include <cstring>

#include "terminal.h"

namespace vterm {


	/** Pseudoterminal base api which specifies blocking send and receive methods. 
	 */
	class PTY {
	protected:
		friend class Terminal::PTYBackend;

		/** Called when data should be sent to the target process. 

		    Sends the buffer of given size to the target process. Returns the number of bytes sent, which is identical to the size of the buffer given unless there was a failure. 
		 */
		virtual size_t sendData(char * buffer, size_t size) = 0;

		/** Waits for the target process to send data and populates the given buffer. 

		    Up to availableSize bytes can be read at once, but the actual number of bytes received is to be returned by the function. 
		 */
		virtual size_t receiveData(char * buffer, size_t availableSize) = 0;

		/** Notifies the underlying terminal process that the terminal size has changed to given values. 
		 */
		virtual void resize(unsigned cols, unsigned rows) = 0;

	}; // vterm::PTY

} // namespace vterm