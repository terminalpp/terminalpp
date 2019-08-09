#pragma once

#ifdef ARCH_WINDOWS
#include "windows.h"
#endif

#include "pty.h"
#include "helpers/process.h"

namespace vterm {

	/** Pseudoterminal to a local process.

	    Implements a pseudoterminal API for local OS process. 
	 */
	class LocalPTY : public PTY {
	public:

		void resize(unsigned cols, unsigned rows) override;

		/** Starts the local pseudoterminal for given command. 
		 */
		LocalPTY(helpers::Command const& command);
		LocalPTY(helpers::Command const& command, helpers::Environment const& env);

		~LocalPTY() override;

	protected:


		// PTY interface implementation

		size_t doWrite(char const* buffer, size_t size) override;
		size_t doRead(char* buffer, size_t availableSize) override;
		void doTerminate() override;
		helpers::ExitCode doWaitFor() override;



	private:
		helpers::Command command_;
		helpers::Environment environment_;

#if (defined ARCH_WINDOWS)

		/** Opens the pipes and creates a new pseudoconsole. 
		 */
		void createPseudoConsole();

		/** Starts the process with the specified command. 
		 */
		void start();

		/** Startup info for the process.
		 */
		STARTUPINFOEX startupInfo_;

		/** Handle to the ConPTY object created for the command.
		 */
		HPCON conPTY_;

		/** The pipe from which input should be read.
		 */
		HANDLE pipeIn_;

		/** Pipe to which data for the application should be sent.
		 */
		HANDLE pipeOut_;

		/** Information about the process being executed.
		 */
		PROCESS_INFORMATION pInfo_;

#elif (defined ARCH_UNIX)

		static constexpr pid_t IGNORE_TERMINATION = 0;

		void start();

		int pipe_;

		pid_t pid_;

#else
#error "Unsupported platform"
#endif

	}; // vterm::LocalPTY


} // namespace vterm