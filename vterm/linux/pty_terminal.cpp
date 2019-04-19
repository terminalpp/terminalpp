#ifdef __linux__

#include <unistd.h>
#include <pty.h>

#include "helpers/log.h"

#include "pty_terminal.h"

namespace vterm {

	PTYTerminal::PTYTerminal(std::string const & command, unsigned cols, unsigned rows):
		IOTerminal(cols, rows),
		command_{ command } {
	}

	void PTYTerminal::execute() {
		// fork & open the pty
		switch (pid_ = forkpty(&pipe_, nullptr, nullptr, nullptr)) {
			// forkpty failed
			case -1:
				NOT_IMPLEMENTED; // an error
		    // running the child process,
			case 0: {
				printf("Oh my god!\n");
				while (true) {
					printf("haha\n");
					sleep(10);
				}
			}
			// continuing the terminal program 
			default:
				LOG << "Executed, pid " << pid_;
				IOTerminal::doStart();
				break;
		}
	}

	bool PTYTerminal::readInputStream(char* buffer, size_t& size) {
		LOG << "Reading from child process";
		int cnt = read(pipe_, (void*)buffer, size);
		if (cnt < 0)
			NOT_IMPLEMENTED;
		size = cnt;
		LOG << "Read " << cnt << " bytes";
		return true;
	}

	void PTYTerminal::doResize(unsigned cols, unsigned rows) {
		return;
	}

	bool PTYTerminal::write(char const* buffer, size_t size) {
		return true;
	}

}


#endif