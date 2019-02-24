#pragma once

#include "vterm/virtual_terminal.h"

namespace tpp {

	typedef vterm::VirtualTerminal<vterm::Encoding::UTF16> Terminal;

	class TerminalWindow;

	class Application {
	public:

		virtual ~Application() {
		}

		virtual TerminalWindow * createNewTerminalWindow() = 0;

		virtual void mainLoop() = 0;

	protected:
		Application() {
		}
	};


} // namespace tpp