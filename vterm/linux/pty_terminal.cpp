#ifdef __linux__

#include <unistd.h>
#include <pty.h>

#include "helpers/log.h"

#include "pty_terminal.h"

namespace vterm {

	PTYTerminal::PTYTerminal(std::string const & command, std::initializer_list<std::string> args, unsigned cols, unsigned rows):
		IOTerminal(cols, rows),
		command_{ command },
		args_{ args } {
	}

	void PTYTerminal::execute() {
		// fork & open the pty
		switch (pid_ = forkpty(&pipe_, nullptr, nullptr, nullptr)) {
			// forkpty failed
			case -1:
				LOG << "fork fail";
				NOT_IMPLEMENTED; // an error
		    // running the child process,
			case 0: {
				//system("ls -la");
				
				char** args = new char* [args_.size() + 2];
				args[0] = const_cast<char*>(command_.c_str());
				for (size_t i = 0; i < args_.size(); ++i)
					args[i + 1] = const_cast<char*>(args_[i].c_str());
				
				args[args_.size() + 1] = nullptr;
				// execvp never returns
				execvp(command_.c_str(), args);
				printf("Hello\n");
				while (true) {};
				NOT_IMPLEMENTED;
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
		if (cnt < 0) {
			LOG << strerror(errno) << " - " << cnt;
			size = 0;
			return false;
		}
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