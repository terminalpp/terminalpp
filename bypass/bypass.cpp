#include <cstdlib>
#include <unistd.h>
#include <iostream>
#include <thread>
#include <termios.h>
#include <sys/ioctl.h>
#include <signal.h>

#include "helpers/helpers.h"
#include "helpers/char.h"
#include "helpers/log.h"
#include "helpers/args.h"

#include "vterm/local_pty.h"

/** Size of the communications buffer. 
 */
helpers::Arg<unsigned> BufferSize({ "--buffer-size" }, 10240, false, "Size of the communications buffer");

/** The command to be executed. 
 */
helpers::Arg<std::vector<std::string>> Command({ "-e" }, {}, true, "Command to be executed in the opened PTY and its arguments", true);

/** .
 */
class PTYEncoder {
public:

	static constexpr char const * PTYENC = "PYENC";

    PTYEncoder(helpers::Command const & cmd, helpers::Environment const & env):
        command_(cmd),
		environment_(env),
        pty_(command_, environment_) {
        Singleton() = this;
        signal(SIGWINCH, SIGWINCH_handler);
        // start the pty reader and encoder thread
        outputEncoder_ = std::thread([this]() {
            char * buffer = new char[* BufferSize];
            size_t numBytes;
            while (true) {
                numBytes = pty_.read(buffer, * BufferSize);
                // if nothing was read, the process has terminated and so should we
                if (numBytes == 0)
                    break;
                //encodeOutput(buffer, numBytes);
				write(STDOUT_FILENO, (void*)buffer, numBytes);
			}
            delete [] buffer;
        });
        // start the cin reader and encoder thread
        std::thread inputEncoder([this]() {
            char * buffer = new char[* BufferSize];
            char * bufferWrite = buffer;
            size_t numBytes;
            while (true) {
                numBytes = read(STDIN_FILENO, (void *) bufferWrite, *BufferSize - (bufferWrite - buffer));
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
        });
        inputEncoder.detach();

    }

    ~PTYEncoder() {
        signal(SIGWINCH, SIG_DFL);
    }

    helpers::ExitCode waitForDone() {
        helpers::ExitCode ec = pty_.waitFor();
        // close stdin so that the inputEncoder will stop
        //close(STDIN_FILENO);
        outputEncoder_.join();
        //inputEncoder.join();
        return ec;
    }

	void resize(unsigned cols, unsigned rows) {
		pty_.resize(cols, rows);
	}

private:

    static PTYEncoder * & Singleton() {
        static PTYEncoder * instance;
        return instance;
    }

    static void SIGWINCH_handler(int signum) {
        MARK_AS_UNUSED(signum);
        PTYEncoder * enc = Singleton();
        if (enc != nullptr) {
            struct winsize ws;
            if (ioctl(STDOUT_FILENO, TIOCGWINSZ, & ws) == -1 || ws.ws_col == 0)
                UNREACHABLE;
            enc->pty_.resize(ws.ws_col, ws.ws_row);
        }
    }

    /** Input comes encoded and must be decoded and sent to the pty. 
     */
    size_t decodeInput(char * buffer, size_t bufferSize) {
#define WRITE(FROM, TO) if (FROM != TO) { pty_.write(buffer + FROM, TO - FROM); FROM = TO; }
#define NEXT if (++i == bufferSize) return processed
#define NUMBER(VAR) if (!ParseNumber(buffer, bufferSize, i, VAR)) return processed
#define POP(WHAT) if (buffer[i++] != WHAT) { LOG(PTYENC) << "Expected " << WHAT << " but " << buffer[i] << " found"; processed = i; continue; }
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
						LOG(PTYENC) << "Unrecognized command " << buffer[i];
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
		while (helpers::IsDecimalDigit(buffer[i])) {
			value = value * 10 + (buffer[i] - '0');
			if (++i == bufferSize)
				return false;
		}
		return i != bufferSize; // at least one valid character must be present after the number
	}


    helpers::Command command_;
	helpers::Environment environment_;
    vterm::LocalPTY pty_;
    std::thread outputEncoder_;


}; // PTYEncoder

int main(int argc, char * argv[]) {
	helpers::Arguments::SetDescription(R"xxx(
ConPTY Bypass for WSL

Simple program which creates a linux pseudoterminal and executes in it the given command, redirecting its output to own output. Passes own input to the created pseudoterminal unless the input contains specific sequences upon which the bypass updates the pseudoterminal accordingly. 
)xxx");
	helpers::Arguments::SetUsage(R"xxx(
bypass [--buffer-size=<n>] { envVar=value } -e ...

Where the envVar=value are key-value pairs to be set in the environment of the process specified by argument -e.
)xxx");
	helpers::Arguments::AllowUnknownArguments();
	helpers::Arguments::Parse(argc, argv);
    try {
		helpers::Command cmd(*Command);
		helpers::Environment env(helpers::Arguments::UnknownArguments());
        PTYEncoder enc(cmd, env);
        return enc.waitForDone();
    } catch (helpers::Exception const & e) {
        std::cerr << "ptyencoder error: " << std::endl << e << std::endl;
    }
    return EXIT_FAILURE;
}
