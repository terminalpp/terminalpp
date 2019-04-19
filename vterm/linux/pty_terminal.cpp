#ifdef __linux__

#include <pty.h>

#include "pty_terminal.h"

namespace vterm {

	PTYTerminal::PTYTerminal(std::string const & command, unsigned cols, unsigned rows):
		IOTerminal(cols, rows),
		command_{ command } {
	}

	void PTYTerminal::execute() {
		if (openpty(&pipeIn_, &pipeOut_, nullptr, nullptr, nullptr) > 0)
			NOT_IMPLEMENTED; // throw proper error using strerror(errno)
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