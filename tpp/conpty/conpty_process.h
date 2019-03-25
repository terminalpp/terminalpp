#ifdef HAHA

#pragma once

#include <windows.h>

#include "vterm/terminal.h"

namespace tpp {

	/** Terminal process using the Windows Pseudo Console 

	    While console emulation was really painful in older versions of Windows, fortunately in Win10 starting Fall/2018, the ConPTY API simplifies things a lot. The following blog describes both the old problems as well as the ConPTY API and how it changes things:

		https://blogs.msdn.microsoft.com/commandline/2018/08/02/windows-command-line-introducing-the-windows-pseudo-console-conpty/

		TODO add terminal resize function...

	 */
	class ConPTYProcess : public vterm::PTYTerminal::Process {
	public:
		ConPTYProcess(std::string const & command, vterm::PTYTerminal * terminal);


	protected:
		void resize(unsigned width, unsigned height) override {

		}

	private:

		/** Opens the pipes and creates a new pseudoconsole. 
		 */
		void createPseudoConsole();

		/** Executes the command, directing its output to the pseudoconsole. 
		 */
		void execute();

	};

} // namespace tpp

#endif