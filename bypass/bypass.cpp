
#include <cstdlib>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <termios.h>
#include <memory.h>
#if (defined ARCH_LINUX)
	#include <pty.h>
#elif (defined ARCH_MACOS)
	#include <util.h>
#endif

#include <iostream>
#include <thread>
#include <string>
#include <vector>
#include <unordered_map>

class Bypass {
public:
    /** Initializes the bypass and parses its command line arguments. 
	 */
    Bypass(int argc, char * argv[]):
	    bufferSize_{10240} {
		int i = 1;
		for (; i < argc; ++i) {
			std::string arg = argv[i];
			if (arg == "-e") {
				for (++i; i < argc; ++i)
				    cmd_.push_back(argv[i]);
				// if the command is empty, none was supplied, break to the error
				if (cmd_.empty())
				    break;
				return;
			} else if (arg.find("--buffer-size") == 0) {
				if (arg[13] == '=') {
					bufferSize_ = std::stoul(arg.substr(14));
				} else {
					if (++i == argc)
					    throw std::runtime_error("Missing buffer size value (and command to execute)");
					arg = argv[i];
					bufferSize_ = std::stoul(arg);
				}
			} else {
				size_t assignPos = arg.find("=");
				if (assignPos == std::string::npos)
				    std::runtime_error(std::string("Invalid argument: ") + arg);
				env_.insert(std::make_pair(arg.substr(0, assignPos), arg.substr(assignPos + 1)));
			}
		}
		throw std::runtime_error("No command to execute specified (missing '-e' arg)");
	}

	/** Executes the command and relays its I/O.
	 */
	int run() {
		switch (pid_ = forkpty(&pipe_, nullptr, nullptr, nullptr)) {
			case -1:
			    throw std::runtime_error("Fork failed");
			// child process
			case 0: {
			    setsid();
				if (ioctl(1, TIOCSCTTY, nullptr) < 0)
				    throw std::runtime_error("Unable to reach terminal in child");
			    setTargetEnvironment();
				clearTargetSignals(); 
				char ** argv = commandToArgv();
				if (execvp(cmd_.front().c_str(), argv) != -1)
				    throw std::runtime_error("Unable to execute target command");
				// this cannot happen
				throw std::runtime_error("");				
			}
			default:
			    return translate();
		}
	}

private:

	char ** commandToArgv() {
		char** args = new char* [cmd_.size() + 1];
		for (size_t i = 0; i < cmd_.size(); ++i)
		    args[i] = const_cast<char*>(cmd_[i].c_str());
		args[cmd_.size()] = nullptr;
		return args;
	}

	void setTargetEnvironment() {
		unsetenv("COLUMNS");
		unsetenv("LINES");
		unsetenv("TERMCAP");
		if (env_.find("TERM") == env_.end())
			setenv("TERM", "xterm-256color", true);
		if (env_.find("COLORTERM") == env_.end())
			setenv("COLORTERM", "truecolor", true);
		for (auto i : env_)
			setenv(i.first.c_str(), i.second.c_str(), true);
	}

	void clearTargetSignals() {
		signal(SIGCHLD, SIG_DFL);
		signal(SIGHUP, SIG_DFL);
		signal(SIGINT, SIG_DFL);
		signal(SIGQUIT, SIG_DFL);
		signal(SIGTERM, SIG_DFL);
		signal(SIGALRM, SIG_DFL);
	}

	void resize(int cols, int rows) {
        struct winsize s;
        s.ws_row = rows;
        s.ws_col = cols;
        s.ws_xpixel = 0;
        s.ws_ypixel = 0;
        if (ioctl(pipe_, TIOCSWINSZ, &s) < 0)
            throw std::runtime_error("Unable to resize target terminal");
	}

	int translate() {
		std::thread outputBypass{[this]() {
			char * buffer = new char [bufferSize_];
			while (true) {
				int numBytes = 0;
				while (true) {
					numBytes = read(pipe_, (void*)buffer, bufferSize_);
					if (numBytes == -1) {
						if (errno == EINTR || errno == EAGAIN)
						    continue;
						numBytes = 0;
					}
					break;
				}
				if (numBytes == 0)
				    break;
				write(STDOUT_FILENO, (void*)buffer, numBytes);
			}
            delete [] buffer;
		}};
		std::thread inputDecoder{[this]() {
            char * buffer = new char[bufferSize_];
            char * bufferWrite = buffer;
            while (true) {
                size_t numBytes = read(STDIN_FILENO, (void *) bufferWrite, bufferSize_ - (bufferWrite - buffer));
                if (numBytes == 0)
                    break;
                numBytes += bufferWrite - buffer;
                size_t processed = decodeInput(buffer, numBytes);
                if (processed != numBytes) {
                    memcpy(buffer, buffer + processed, numBytes - processed);
                    bufferWrite = buffer + (numBytes - processed);
                } else {
                    bufferWrite = buffer;
                }
            }
            delete [] buffer;

		}};
		inputDecoder.detach();
		outputBypass.join();
		int ec;
		pid_t x = waitpid(pid_, &ec, 0);
		ec = WEXITSTATUS(ec);
		if (x < 0 && errno != ECHILD)
		    throw std::runtime_error("Unable to wait for target process termination.");
		return ec;
	}

    /** Input comes encoded and must be decoded and sent to the pty. 
     */
    size_t decodeInput(char * buffer, size_t bufferSize) {
		
#define WRITE(FROM, TO) if (FROM != TO) { write(pipe_, (void*)(buffer + FROM), TO - FROM); FROM = TO; }
#define NEXT if (++i == bufferSize) return processed
#define NUMBER(VAR) if (!ParseNumber(buffer, bufferSize, i, VAR)) return processed
#define POP(WHAT) if (buffer[i++] != WHAT) { throw std::runtime_error(std::string("Expected ") + #WHAT + ", but found " + buffer[i]); }
		size_t processed = 0;
		size_t start = 0;
		while (processed < bufferSize) {
			if (buffer[processed] == '`') {
				WRITE(start, processed);
				size_t i = processed;
				NEXT;
				switch (buffer[i]) {
					// if the character after backtick is backtick, the second backtick will be the beginning of next batch
					case '`':
						start = i;
						processed = i + 1;
						continue;
					// the resize command (`r COLS : ROWS ;)
					case 'r': {
						unsigned cols;
						unsigned rows;
						NEXT;
						NUMBER(cols);
						POP(':');
						NUMBER(rows);
						POP(';');
						resize(cols, rows);
						processed = i;
						start = processed;
						continue;
					// otherwise (unrecognized command) skip what we have and move on
					default:
					    throw std::runtime_error(std::string("Unrecognized command") + buffer[i]);
						processed = i + 1;
						start = processed;
						continue;
					}
				}
			} 
			++processed;
		}
		WRITE(start, processed);
		return processed;
#undef WRITE
#undef NEXT
#undef NUMBER
#undef POP
    }

	static bool ParseNumber(char* buffer, size_t bufferSize, size_t& i, unsigned& value) {
		value = 0;
		while (buffer[i] >= '0' && buffer[i] <= '9') {
			value = value * 10 + (buffer[i] - '0');
			if (++i == bufferSize)
				return false;
		}
		return i != bufferSize; // at least one valid character must be present after the number
	}

    std::vector<std::string> cmd_;
	std::unordered_map<std::string, std::string> env_;
	unsigned bufferSize_;

    pid_t pid_;
	int pipe_;
	
    

}; // Bypass


int main(int argc, char * argv[]) {
	try {
		Bypass bypass(argc, argv);
		try {
			bypass.run();
			return EXIT_SUCCESS;
		} catch (std::exception const & e) {
			std::cerr << "Bypass terminated with error: " <<  e.what() << std::endl;
			// TODO add the error to some logfile
		}
	} catch (std::exception const & e) {
		// TODO do usage

		std::cerr << "Bypass error: " << e.what() << std::endl;
	}
	return EXIT_FAILURE;
}
