#pragma once

#ifdef _WIN64
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

		size_t write(char const * buffer, size_t size) override;
		size_t read(char* buffer, size_t availableSize) override;

		void resize(unsigned cols, unsigned rows) override;


		/** Starts the local pseudoterminal for given command. 
		 */
		LocalPTY(helpers::Command const& command);
		LocalPTY(helpers::Command const& command, helpers::Environment const& env);

		~LocalPTY() override;

	protected:

		// PTY interface implementation

		/** Terminates the process immediately. 
		 */
		void doTerminate() override;

		helpers::ExitCode doWaitFor() override;



	private:
		helpers::Command command_;
		helpers::Environment environment_;

#ifdef _WIN64

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

#elif __linux__

		static constexpr pid_t IGNORE_TERMINATION = 0;

		void start();

		int pipe_;

		pid_t pid_;

#else
#error "Unsupported platform"
#endif

	}; // vterm::LocalPTY


} // namespace vterm