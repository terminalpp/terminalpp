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
		LocalPTY(std::string const& command);

		~LocalPTY() override;

	protected:

		// PTY interface implementation

		size_t sendData(char const * buffer, size_t size) override;
		size_t receiveData(char* buffer, size_t availableSize) override;
		void resize(unsigned cols, unsigned rows) override;


	private:
		std::string command_;


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

#endif





	}; // vterm::LocalPTY


} // namespace vterm