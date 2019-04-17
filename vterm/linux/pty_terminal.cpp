#ifdef __linux__

#include "pty_terminal.h"

namespace vterm {

	PTYTerminal::PTYTerminal(std::string const & command, unsigned cols, unsigned rows):
		IOTerminal(cols, rows),
		command_{ command } {
	}

	bool PTYTerminal::readInputStream(char* buffer, size_t& size) {
		return false;
	}

	void PTYTerminal::doResize(unsigned cols, unsigned rows) {
		return;
	}

	bool PTYTerminal::write(char const* buffer, size_t size) {
		return false;
	}

}


#endif