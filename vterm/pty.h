#pragma once

#include <cstring>

#include "helpers/object.h"

#include "terminal.h"

namespace vterm {

	/** Pseudoterminal base api which specifies blocking send and receive methods. 
	 */
	class PTY {
	public:

		/** Because different operating systems support different exit code types, we define the type exit code separately. 
		 */
#ifdef WIN32
		typedef unsigned long ExitCode;
#elif __linux__
		typedef int ExitCode;
#endif

		/** Immediately terminates the attached process. 

		    Has no effect if the process has already terminated. 
		 */
		void terminate() {
			if (terminated_)
				return;
			doTerminate();
		}

		/** Blocks the current thread, waiting for the attached process to terminate. 
		
		    When done, returns the exit code of the process. Waiting for process that is not running should immediately return the exit code. 
		 */
		ExitCode waitFor() {
			if (!terminated_)
				exitCode_ = doWaitFor();
			return exitCode_;
		}

		/** Virtual destructor so that resources are properly deleted. 

		    Children of PTY are expected to terminate the attached procesa and clear all resources here.
		 */
		virtual ~PTY() {
		}

	protected:
		friend class Terminal::PTYBackend;

		PTY() :
			terminated_(false) {
		}

		/** Terminates the attached process. 
		 */
		virtual void doTerminate() = 0;

		/** Waits for the attached process to terminate and then returns the exit code. 
		 */
		virtual ExitCode doWaitFor() = 0;

		/** Called when data should be sent to the target process. 

		    Sends the buffer of given size to the target process. Returns the number of bytes sent, which is identical to the size of the buffer given unless there was a failure. 
		 */
		virtual size_t sendData(char const * buffer, size_t size) = 0;

		/** Returns the number of bytes available to read.
		 */
		virtual size_t receiveDataReady() = 0;

		/** Waits for the target process to send data and populates the given buffer.

		    Up to availableSize bytes can be read at once, but the actual number of bytes received is to be returned by the function. 
		 */
		virtual size_t receiveData(char * buffer, size_t availableSize) = 0;


		/** Notifies the underlying terminal process that the terminal size has changed to given values. 
		 */
		virtual void resize(unsigned cols, unsigned rows) = 0;


		void markAsTerminated(int exitCode) {
			ASSERT(! terminated_);
			terminated_ = true;
			exitCode_ = exitCode;
		}

		bool terminated_;
		ExitCode exitCode_;

	}; // vterm::PTY

} // namespace vterm