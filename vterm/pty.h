#pragma once

#include <cstring>

#include <mutex>
#include <condition_variable>

#include "helpers/process.h"
#include "helpers/object.h"

#include "terminal.h"

namespace vterm {

	/** Pseudoterminal base api.

	    The PTY defines the minimal viable API a pseudoterminal must support. All the supported actions are synchronous, i.e. it is up to their callers to determine whether asynchronous operation is to be emulated via multiple threads. While this is slightly less efficient than asynchronous operations, it creates much simpler interface common across operating systems and pseudoterminal target. 

		A pseudoterminal is presumed to be attached to a process (this may be local process, a server, etc.) and must support terminating the attached process (where termination makes sense, or disconnecting from it), waiting for the process to terminate on its own, sending and receiving data and trigerring the resize event of the attached process.

		PTY for locally running processes is implemented in the `local_pty.h` file. 
	 */
	class PTY {
	public:

		/** Immediately terminates the attached process. 

		    Has no effect if the process has already terminated. 
		 */
		void terminate() {
			{
				std::lock_guard<std::mutex> g(m_);
				if (!terminated_)
					doTerminate();
			}
			waitFor();
		}

		/** Blocks the current thread, waiting for the attached process to terminate. 
		
		    When done, returns the exit code of the process. If the process has already exited or was terminated, returns the exit code immediately.
		 */
		helpers::ExitCode waitFor() {
			std::unique_lock<std::mutex> g(m_);
			while (!terminated_)
				cv_.wait(g);
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
			terminated_(false),
		    exitCode_(0) {
		}

		/** Monitors the attached process and manages the termination and exit code statuses. 
		 */
		void monitor() {
			std::thread t([this]() {
				helpers::ExitCode ec = doWaitFor();
				{
					std::lock_guard<std::mutex> g(m_);
					terminated_ = true;
					exitCode_ = ec;
					cv_.notify_all();
				}
			});
			t.detach();
		}

		/** Terminates the attached process. 

		    Terminates the attached process. Should immediately return if the process has already finished on its own, or has been terminated.

			Can block the calling thread, but this behavior is not required because the `terminate()` method from the public API blocks on the internal monitor to observe the termination of the process before returning.
		 */
		virtual void doTerminate() = 0;

		/** Waits for the attached process to terminate and then returns the exit code. 
		 */
		virtual helpers::ExitCode doWaitFor() = 0;

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


		void markAsTerminated(int exitCode) {
			ASSERT(! terminated_);
			terminated_ = true;
			exitCode_ = exitCode;
		}

		bool terminated_;
		helpers::ExitCode exitCode_;

		std::mutex m_;
		std::condition_variable cv_;
	}; // vterm::PTY

} // namespace vterm