
#include <cstdlib>
#include <cassert>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <pwd.h>
#include <errno.h>
#include <termios.h>
#include <memory.h>
#include <pty.h>

#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>
#include <string>
#include <vector>
#include <unordered_map>

#include "stamp.h"

/** The Windows ConPTY bypass via WSL
 
    The bypass creates a pseudoterminal in the WSL and relays any traffic on that terminal unchanged to the terminal connected via standard input and output, thus bypassing the Win32 ConPTY and its encoding and decoding of the escape sequences. This allows the terminal to use the terminal for linux applications in the same way it would on linux and spares it any issues the ConPTY might have. 

	Extra terminal commands, such as terminal resize events are encoded in the stream using the backtick escape character.

	An additional benefit is increase in speed since the ConPTY has to do much than the simple bypass. 
 */
class Bypass {
public:

    /** Initializes the bypass and parses its command line arguments. 
	 */
    Bypass(int argc, char * argv[]):
	    bufferSize_{10240},
		pipe_{0} {
		int i = 1;
		for (; i < argc; ++i) {
			std::string arg = argv[i];
			if (arg == "-e") {
				for (++i; i < argc; ++i)
				    cmd_.push_back(argv[i]);
				// if the command is empty, none was supplied, error
				if (cmd_.empty())
					throw std::runtime_error("No command to execute specified after -e argument");
				return;
			} else if (arg.starts_with("--buffer-size")) {
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
		// if not command was specified, use the default shell of the current user
		assert(cmd_.empty());
		cmd_.push_back(getpwuid(getuid())->pw_shell);

	}

	/** Executes the command and relays its I/O.

	    When the command terminates, returns its exit code.  
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

	/** Converts the command from the commandline to the null terminated array of null terminated strings required by the execvp. 
	 */
	char ** commandToArgv() {
		char** args = new char* [cmd_.size() + 1];
		for (size_t i = 0; i < cmd_.size(); ++i)
		    args[i] = const_cast<char*>(cmd_[i].c_str());
		args[cmd_.size()] = nullptr;
		return args;
	}

	/** Sets the taregt command environment. 
	 
	    Clears any interfering definitions and updates the environment to act as a proper terminal (i.e. terminfo, color profile, shell, etc.). Applies any environment changes passed on the commandline as well. 
	 */
	void setTargetEnvironment() {
		unsetenv("COLUMNS");
		unsetenv("LINES");
		unsetenv("TERMCAP");
		if (env_.find("SHELL") == env_.end())
		    setenv("SHELL", getpwuid(getuid())->pw_shell, true);
		if (env_.find("TERM") == env_.end())
			setenv("TERM", "xterm-256color", true);
		if (env_.find("COLORTERM") == env_.end())
			setenv("COLORTERM", "truecolor", true);
		for (auto i : env_)
			setenv(i.first.c_str(), i.second.c_str(), true);
	}

	/** Clears the signal handling in the target process. 
	 */
	void clearTargetSignals() {
		signal(SIGCHLD, SIG_DFL);
		signal(SIGHUP, SIG_DFL);
		signal(SIGINT, SIG_DFL);
		signal(SIGQUIT, SIG_DFL);
		signal(SIGTERM, SIG_DFL);
		signal(SIGALRM, SIG_DFL);
	}

	/** Resizes the terminal to the target command. 
	 */
	void resize(int cols, int rows) {
        struct winsize s;
        s.ws_row = rows;
        s.ws_col = cols;
        s.ws_xpixel = 0;
        s.ws_ypixel = 0;
        if (ioctl(pipe_, TIOCSWINSZ, &s) < 0)
            throw std::runtime_error("Unable to resize target terminal");
	}

    /** Reads the output of the command in the terminal pipe and outputs it unchanged on the stdout, reads the stdin, translates any extra commands (terminal resize) and passes the rest as input to the target commands's pseudoterminal.
	    
		When done, returns the exit code of the target command. 
	 */
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
					// otherwise (unrecognized command) do an error
					default:
					    throw std::runtime_error(std::string("Unrecognized command") + buffer[i]);
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
	if (argc == 2 && strncmp(argv[1], "--version", 10) == 0) {
        std::cout << "ConPTY bypass for terminal++, version " << stamp::version << std::endl;
        std::cout << "    commit:   " << stamp::commit << (stamp::dirty ? "*" : "") << std::endl;
        std::cout << "              " << stamp::build_time << std::endl;
        std::cout << "    platform: " << stamp::arch << " " << stamp::arch_size << " " << stamp::arch_compiler << " " << stamp::arch_compiler_version << " " << stamp::build << std::endl;
		return EXIT_SUCCESS;
	}
	try {
		Bypass bypass(argc, argv);
		try {
			bypass.run();
			return EXIT_SUCCESS;
		} catch (std::exception const & e) {
			std::cerr << "Bypass terminated with error: " <<  e.what() << std::endl;
		}
	} catch (std::exception const & e) {
		std::cerr << "ConPTY Bypass for t++. Usage: " << std::endl << std::endl;
		std::cerr << "tpp-bypass {--buffer-size | envVar=value } [ -e cmd { arg }]" << std::endl << std::endl;
		std::cerr << "Where:" << std::endl;
		std::cerr << "   --buffer-size determines the sizes of the I/O byuffers (--bufferSize=1024)" << std::endl;
		std::cerr << "   envVar=value sets given environment variable to the value before executing the command" << std::endl;
		std::cerr << "   -e sets the command to execute (defaults to current users's shell)" << std::endl;
		std::cerr << "Bypass error: " << e.what() << std::endl;
	}
	return EXIT_FAILURE;
}
