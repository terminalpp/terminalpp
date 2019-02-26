#pragma once

#include <windows.h>

#include "vterm/virtual_terminal.h"

namespace tpp {

	/** Terminal connector using the Windows Pseudo Console 

	    While console emulation was really painful in older versions of Windows, fortunately in Win10 starting Fall/2018, the ConPTY API simplifies things a lot. The following blog describes both the old problems as well as the ConPTY API and how it changes things:

		https://blogs.msdn.microsoft.com/commandline/2018/08/02/windows-command-line-introducing-the-windows-pseudo-console-conpty/

		TODO add terminal resize function...

	 */
	class ConPTYConnector : public vterm::VirtualTerminal::Connector {
	public:
		ConPTYConnector(std::string const & command, vterm::VirtualTerminal * terminal);




		~ConPTYConnector() {
			if (conPTY_ != INVALID_HANDLE_VALUE)
			    ClosePseudoConsole(conPTY_);
			if (pipeIn_ != INVALID_HANDLE_VALUE)
				CloseHandle(pipeIn_);
			if (pipeOut_ != INVALID_HANDLE_VALUE)
				CloseHandle(pipeOut_);
			free(startupInfo_.lpAttributeList);
		}

	private:

		/** Opens the pipes and creates a new pseudoconsole. 
		 */
		void createPseudoConsole();

		/** Executes the command, directing its output to the pseudoconsole. 
		 */
		void execute();

		static void InputPipeReader(ConPTYConnector & connector);

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





} // namespace tpp