#pragma once

#ifdef WIN32
#include <windows.h>
#endif

#include "pty.h"

namespace vterm {

	class LocalPTY : public PTY {
	public:

		/** Starts the local pseudoterminal for given command. 
		 */
		LocalPTY(std::string const& command, std::initializer_list<std::string> args);

		~LocalPTY() override;

		/** Terminates the underlying process.
		 */
		void terminate() override;

	protected:

		// PTY interface implementation

		size_t sendData(char const * buffer, size_t size) override;
		size_t receiveData(char* buffer, size_t availableSize) override;
		void resize(unsigned cols, unsigned rows) override;


	private:
		std::string command_;

		std::vector<std::string> args_;

		/** Thread which waits for the attached process to terminate. 
		 */
		std::thread t_;

#ifdef WIN32

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