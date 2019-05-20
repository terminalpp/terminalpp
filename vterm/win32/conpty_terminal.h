#pragma once
#ifdef WIN32__
#include <windows.h>

#include "../terminal.h"

namespace vterm {

	class ConPTYTerminal : public virtual IOTerminal {
	public:
		ConPTYTerminal(std::string const & command, unsigned cols, unsigned rows);

		~ConPTYTerminal() {
			if (conPTY_ != INVALID_HANDLE_VALUE)
				ClosePseudoConsole(conPTY_);
			if (pipeIn_ != INVALID_HANDLE_VALUE)
				CloseHandle(pipeIn_);
			if (pipeOut_ != INVALID_HANDLE_VALUE)
				CloseHandle(pipeOut_);
			free(startupInfo_.lpAttributeList);
		}

		void execute() {
			doStart();
		}

	protected:

		/** Opens the pipes and creates a new pseudoconsole.
		 */
		void createPseudoConsole();

		void doStart() override;

		bool readInputStream(char * buffer, size_t & size) override;

		void doResize(unsigned cols, unsigned rows) override;

		bool write(char const * buffer, size_t size) override;

	private:
		/** The command executed by the connector.

			Keeping the command as property also "satisfies" the fact that CreateProcess requires mutable command line as its argument.
		 */
		std::string command_;

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


	};

} // namespace vterm

#endif