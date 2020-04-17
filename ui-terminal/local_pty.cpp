#if (defined ARCH_UNIX)
    #include <unistd.h>
    #include <signal.h>
    #include <sys/wait.h>
    #include <sys/ioctl.h>
    #include <errno.h>
    #if (defined ARCH_LINUX)
        #include <pty.h>
    #elif (defined ARCH_MACOS)
        #include <util.h>
    #endif
#endif

#include <memory>

#include "helpers/string.h"
#include "helpers/locks.h"
#include "helpers/log.h"

#include "local_pty.h"

namespace ui {

#if (defined ARCH_WINDOWS)    

    LocalPTY::LocalPTY(Client * client, helpers::Command const & command):
        IOPTY{client},
        command_{command},
		startupInfo_{},
   		pipeIn_{ INVALID_HANDLE_VALUE },
		pipeOut_{ INVALID_HANDLE_VALUE } {
        start();
    }

    LocalPTY::LocalPTY(Client * client, helpers::Command const & command, helpers::Environment const & env):
        IOPTY{client},
		command_(command),
		environment_(env),
		startupInfo_{},
		pipeIn_{ INVALID_HANDLE_VALUE },
		pipeOut_{ INVALID_HANDLE_VALUE } {
        start();
    }

    LocalPTY::~LocalPTY() {
        // first terminate the process and wait for it
        terminate();
		waiter_.join();
        // then close all handles and the PTY, which interrupts the reader thread
		CloseHandle(pInfo_.hProcess);
		CloseHandle(pInfo_.hThread);
		ClosePseudoConsole(conPTY_);
		CloseHandle(pipeIn_);
		CloseHandle(pipeOut_);
        // finally wait for the reader
		reader_.join();
        // and free the rest of the resources
		delete [] reinterpret_cast<char*>(startupInfo_.lpAttributeList);
    }

    void LocalPTY::terminate() {
        TerminateProcess(pInfo_.hProcess, std::numeric_limits<unsigned>::max());
    }

    void LocalPTY::start() {
		startupInfo_.lpAttributeList = nullptr;
        // create the pseudoconsole
		HRESULT result{ E_UNEXPECTED };
		HANDLE pipePTYIn{ INVALID_HANDLE_VALUE };
		HANDLE pipePTYOut{ INVALID_HANDLE_VALUE };
		// first create the pipes we need, no security arguments and we use default buffer size for now
		OSCHECK(
			CreatePipe(&pipePTYIn, &pipeOut_, NULL, 0) && CreatePipe(&pipeIn_, &pipePTYOut, NULL, 0)
		) << "Unable to create pipes for the subprocess";
		// determine the console size from the terminal we have
		COORD consoleSize{};
		consoleSize.X = 80;
		consoleSize.Y = 25;
		// now create the pseudo console
		result = CreatePseudoConsole(consoleSize, pipePTYIn, pipePTYOut, 0, &conPTY_);
		// delete the pipes on PTYs end, since they are now in conhost and will be deleted when the conpty is deleted
		if (pipePTYIn != INVALID_HANDLE_VALUE)
			CloseHandle(pipePTYIn);
		if (pipePTYOut != INVALID_HANDLE_VALUE)
			CloseHandle(pipePTYOut);
		OSCHECK(result == S_OK) << "Unable to open pseudo console";
        // generate the startup info
		SIZE_T attrListSize = 0;
		startupInfo_.StartupInfo.cb = sizeof(STARTUPINFOEX);
		// allocate the attribute list of required size
		InitializeProcThreadAttributeList(nullptr, 1, 0, &attrListSize); // get size of list of 1 attribute
		startupInfo_.lpAttributeList = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(new char[attrListSize]);
		// initialize the attribute list
		OSCHECK(
			InitializeProcThreadAttributeList(startupInfo_.lpAttributeList, 1, 0, &attrListSize)
		) << "Unable to create attribute list";
		// set the pseudoconsole attribute
		OSCHECK(
			UpdateProcThreadAttribute(
				startupInfo_.lpAttributeList,
				0,
				PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE,
				conPTY_,
				sizeof(HPCON),
				nullptr,
				nullptr
			)
		) << "Unable to set pseudoconsole attribute";
		// finally, create the process with given commandline
		helpers::utf16_string cmd = helpers::UTF8toUTF16(command_.toString());
		OSCHECK(
			CreateProcess(
				nullptr,
				&cmd[0], // the command to execute
				nullptr, // process handle cannot be inherited
				nullptr, // thread handle cannot be inherited
				false, // the new process does not inherit any handles
				EXTENDED_STARTUPINFO_PRESENT, // we have extra info 
				nullptr, // use parent's environment
				nullptr, // use parent's directory
				&startupInfo_.StartupInfo, // startup info
				&pInfo_ // info about the process
			)
		) << "Unable to start process " << command_;

        IOPTY::start();
    }

    void LocalPTY::resize(int cols, int rows) {
		// resize the underlying ConPTY
		COORD size;
		size.X = cols & 0xffff;
		size.Y = rows & 0xffff;
		ResizePseudoConsole(conPTY_, size);
    }

    void LocalPTY::send(char const * buffer, size_t bufferSize) {
		DWORD bytesWritten = 0;
		size_t start = 0;
		size_t i = 0;
        // TODO this is weird, why????
		while (i < bufferSize) {
			if (buffer[i] == '`') {
				WriteFile(pipeOut_, buffer + start, static_cast<DWORD>(i + 1 - start), &bytesWritten, nullptr);
				start = i;
			}
			++i;
		}
		WriteFile(pipeOut_, buffer + start, static_cast<DWORD>(i - start), &bytesWritten, nullptr);
    }

    size_t LocalPTY::receive(char * buffer, size_t bufferSize, bool & success) {
        DWORD bytesRead = 0;
        ASSERT(static_cast<DWORD>(bufferSize) == bufferSize);
        success = ReadFile(pipeIn_, buffer, static_cast<DWORD>(bufferSize), &bytesRead, nullptr);
        return bytesRead;
    }

    helpers::ExitCode LocalPTY::waitAndGetExitCode() {
        while (true) {
            OSCHECK(WaitForSingleObject(pInfo_.hProcess, INFINITE) == 0);
            helpers::ExitCode ec;
            OSCHECK(GetExitCodeProcess(pInfo_.hProcess, &ec) != 0);
            if (ec != STILL_ACTIVE)
                return ec;
        }
    }

#elif (defined ARCH_UNIX)

    LocalPTY::LocalPTY(Client * client, helpers::Command const & command):
        IOPTY{client},
        command_{command} {
        start();

    }

    LocalPTY::LocalPTY(Client * client, helpers::Command const & command, helpers::Environment const & env):
        IOPTY{client},
        command_{command},
        environment_{env} {
        start();
    }

    LocalPTY::~LocalPTY() {
        terminate();
        reader_.join();
        waiter_.join();
    }

    void LocalPTY::terminate() {
        kill(pid_, SIGKILL);
    }

    void LocalPTY::start() {
		// fork & open the pty
		switch (pid_ = forkpty(&pipe_, nullptr, nullptr, nullptr)) {
			// forkpty failed
			case -1:
			    OSCHECK(false) << "Fork failed";
		    // running the child process,
			case 0: {
				setsid();
				if (ioctl(1, TIOCSCTTY, nullptr) < 0)
					UNREACHABLE;
				environment_.unsetIfUnspecified("COLUMNS");
				environment_.unsetIfUnspecified("LINES");
				environment_.unsetIfUnspecified("TERMCAP");
				environment_.setIfUnspecified("TERM", "xterm-256color");
				environment_.setIfUnspecified("COLORTERM", "truecolor");
				environment_.apply();

				signal(SIGCHLD, SIG_DFL);
				signal(SIGHUP, SIG_DFL);
				signal(SIGINT, SIG_DFL);
				signal(SIGQUIT, SIG_DFL);
				signal(SIGTERM, SIG_DFL);
				signal(SIGALRM, SIG_DFL);

				char** argv = command_.toArgv();
				// execvp never returns
				OSCHECK(execvp(command_.command().c_str(), argv) != -1) << "Unable to execute command " << command_;
				UNREACHABLE;
			}
			// continuing the terminal program 
			default:
				break;
		}

        IOPTY::start();
    }

    void LocalPTY::resize(int cols, int rows) {
        struct winsize s;
        s.ws_row = rows;
        s.ws_col = cols;
        s.ws_xpixel = 0;
        s.ws_ypixel = 0;
        if (ioctl(pipe_, TIOCSWINSZ, &s) < 0)
            NOT_IMPLEMENTED;
    }

    void LocalPTY::send(char const * buffer, size_t bufferSize) {
		int nw = ::write(pipe_, (void*)buffer, bufferSize);
		// TODO check errors properly 
		ASSERT(nw >= 0 && static_cast<unsigned>(nw) == bufferSize);
    }

    size_t LocalPTY::receive(char * buffer, size_t bufferSize, bool & success) {
        while (true) {
            int cnt = 0;
            cnt = ::read(pipe_, (void*)buffer, bufferSize);
            if (cnt == -1) {
                if (errno == EINTR || errno == EAGAIN)
                    continue;
                success = false;
                return 0;
            } else {
                success = true;
                return static_cast<size_t>(cnt);
            }
        }
    }

    helpers::ExitCode LocalPTY::waitAndGetExitCode() {
        helpers::ExitCode ec;
        pid_t x = waitpid(pid_, &ec, 0);
        ec = WEXITSTATUS(ec);
        // it is ok to see errno ECHILD, happens when process has already been terminated
        if (x < 0 && errno != ECHILD) 
            NOT_IMPLEMENTED; // error
        return ec;
    }

#endif

} // namespace ui

