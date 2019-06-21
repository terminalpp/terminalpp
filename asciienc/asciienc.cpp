#include <cstdlib>
#include <unistd.h>
#include <iostream>
#include <thread>
#include <termios.h>
#include <sys/ioctl.h>
#include <signal.h>

#include "helpers/helpers.h"
#include "helpers/strings.h"
#include "helpers/log.h"

#include "vterm/local_pty.h"
#include "vterm/ascii_encoder.h"

size_t constexpr BUFFER_SIZE = 10240;

/** RAII raw mode terminal switch.
 */
class RawModeInput {
public:
    RawModeInput() {
        OSCHECK(tcgetattr(STDIN_FILENO, & backup_) != -1) << "Unable to read terminal attributes";
        termios raw = backup_;
        // disable echo mode
        raw.c_lflag &= ~(ECHO);
        // disable canonical mode
        raw.c_lflag &= ~(ICANON);
        // disable C-c and C-z signals
        raw.c_lflag &= ~(ISIG);
        // disable C-s and C-q signals
        raw.c_iflag &= ~(IXON);
        // disable C-v
        raw.c_lflag &= ~(IEXTEN);
        // disable CR newline conversion
        raw.c_iflag &= ~(ICRNL);
        // turn off output processing
        raw.c_oflag &= ~(OPOST);
        // ignore break condition
        raw.c_iflag &= ~(BRKINT);
        // disable parity checking
        raw.c_iflag &= ~(INPCK);
        // disable 8th bit stripping
        raw.c_iflag &= ~(ISTRIP);
        // set character size to 8 bits
        raw.c_cflag |= (CS8);
        // set the terminal properties
        OSCHECK(tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) != -1) << "Unable to enter raw mode";
    }

    ~RawModeInput() noexcept(false) {
        OSCHECK(tcsetattr(STDIN_FILENO, TCSAFLUSH, & backup_) != -1) << "Cannot restore terminal settings";
    }

private:
    termios backup_;
};


/** Executes given process and encodes its input & output into printable ASCII only.

    The encoding is as follows:

    - all printable characters (0x20 - 0x7e inclusive) with the exception of backtick ` (0x60) are encoded as themselves
    - backtick is encoded as two backticks
    - characters from 0x00 to 0x19 are encoded as backtick and character from  
 */
class PTYEncoder : public vterm::ASCIIEncoder::CommandHandler {
public:
    PTYEncoder(helpers::Command const & cmd, helpers::Environment const & env):
        command_(cmd),
		environment_(env),
        pty_(command_, environment_) {
        Singleton() = this;
        signal(SIGWINCH, SIGWINCH_handler);
        // start the pty reader and encoder thread
        outputEncoder_ = std::thread([this]() {
            char * buffer = new char[BUFFER_SIZE];
            size_t numBytes;
            while (true) {
                numBytes = pty_.receiveData(buffer, BUFFER_SIZE);
                // if nothing was read, the process has terminated and so should we
                if (numBytes == 0)
                    break;
                encodeOutput(buffer, numBytes);
            }
            delete [] buffer;
        });
        // start the cin reader and encoder thread
        std::thread inputEncoder([this]() {
            char * buffer = new char[BUFFER_SIZE];
            char * bufferWrite = buffer;
            size_t numBytes;
            while (true) {
                numBytes = read(STDIN_FILENO, (void *) bufferWrite, BUFFER_SIZE - (bufferWrite - buffer));
                if (numBytes == 0)
                    break;
                numBytes += bufferWrite - buffer;
                size_t processed = encodeInput(buffer, numBytes);
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

	void resize(unsigned cols, unsigned rows) override {
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

    /** The process output arrives unencoded on the pty, must be encoded and sent to std out. 

        Since the encoding is done on character by character basis, no state is necessary. 
     */
    void encodeOutput(char * buffer, size_t numBytes) {
        vterm::ASCIIEncoder::Encode(std::cout, buffer, numBytes);
        std::cout << std::flush;
    }

    /** Input comes encoded and must be decoded and sent to the pty. 
     
        
     */
    size_t encodeInput(char * buffer, size_t numBytes) {
        size_t decodedSize;
        size_t result = vterm::ASCIIEncoder::Decode(buffer, numBytes, decodedSize, this);
        pty_.sendData(buffer, decodedSize);
        return result;
    }

    //RawModeInput rmi_;
    helpers::Command command_;
	helpers::Environment environment_;
    vterm::LocalPTY pty_;
    std::thread outputEncoder_;


}; // ASCIIEncoder


std::pair<helpers::Command, helpers::Environment> ParseArguments(int argc, char* argv[]) {
	helpers::Environment env;
	std::vector<std::string> cmdArgs;
	if (argc < 2)
		THROW(helpers::Exception()) << "Invalid number of arguments - at least the command to execute must be specified";
	int i = 1;
	if (std::string(argv[i]) == "-env") {
		++i;
		while (true) {
			if (i == argc)
				break;
			std::string a = argv[i++];
			if (a == "--")
				break;
			auto pos = a.find('=');
			if (pos == std::string::npos)
				THROW(helpers::Exception()) << "Invalid environment variable definition (missing =): " << a;
			env.set(a.substr(0, pos), a.substr(pos + 1));
		}
	}
	while (i < argc) {
		cmdArgs.push_back(argv[i]);
		++i;
	}
	if (cmdArgs.empty())
	    THROW(helpers::Exception()) << "No command to execute given";
	return std::make_pair(helpers::Command(cmdArgs), env);
}

int main(int argc, char * argv[]) {
    try {
		std::pair<helpers::Command, helpers::Environment> args = ParseArguments(argc, argv);
        PTYEncoder enc(args.first, args.second);
        return enc.waitForDone();
    } catch (helpers::Exception const & e) {
        std::cerr << "asciienc error: " << std::endl << e << std::endl;
    }
    return EXIT_FAILURE;
}
