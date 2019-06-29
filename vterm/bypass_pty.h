#pragma once

#ifdef _WIN64
#include "windows.h"
#endif

#include "pty.h"
#include "helpers/process.h"

namespace vterm {

	class BypassPTY : public PTY {
	public:


		/** Starts the local pseudoterminal for given command.
		 */
		BypassPTY(helpers::Command const& command);

		~BypassPTY() override;

		size_t write(char const* buffer, size_t size) override;
		size_t read(char* buffer, size_t availableSize) override;

		void resize(unsigned cols, unsigned rows) override;

	protected:

		// PTY interface implementation

		/** Terminates the process immediately.
		 */
		void doTerminate() override;

		helpers::ExitCode doWaitFor() override;

	private:
		helpers::Command command_;

#ifdef _WIN64

		/** Starts the process with the specified command.
		 */
		void start();

		/** Startup info for the process.
		 */
		STARTUPINFOEX startupInfo_;

		/** The pipe from which input should be read and output pipe handle for the process.
		 */
		HANDLE pipeIn_;
		HANDLE pipePTYOut_;

		/** Pipe to which data for the application should be sent and input pipe handle for the process.
		 */
		HANDLE pipeOut_;
		HANDLE pipePTYIn_;

		/** Information about the process being executed.
		 */
		PROCESS_INFORMATION pInfo_;

#endif

	};


} // namespace vterm