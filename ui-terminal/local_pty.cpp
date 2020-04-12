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

#include "local_pty.h"

namespace ui {

#if (defined ARCH_WINDOWS)    

    LocalPTY::LocalPTY(Client * client, helpers::Command const & command):
        PTY{client},
        command_{command},
		startupInfo_{},
   		pipeIn_{ INVALID_HANDLE_VALUE },
		pipeOut_{ INVALID_HANDLE_VALUE } {
        start();

    }

    LocalPTY::LocalPTY(Client * client, helpers::Command const & command, helpers::Environment const & env):
        PTY{client},
		command_(command),
		environment_(env),
		startupInfo_{},
		pipeIn_{ INVALID_HANDLE_VALUE },
		pipeOut_{ INVALID_HANDLE_VALUE } {
        start();
    }

    LocalPTY::~LocalPTY() {
        terminate();
        reader_.join();
        wait_.join();
        CloseHandle(pInfo_.hProcess);
        CloseHandle(pInfo_.hThread);
		ClosePseudoConsole(conPTY_);
		CloseHandle(pipeIn_);
        CloseHandle(pipeOut_);
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
		size_t attrListSize = 0;
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

        reader_ = std::thread{[this](){
            // create the buffer, and read while we can 
            char * buffer = new char[DEFAULT_BUFFER_SIZE];
            DWORD bytesRead = 0;
            while (true) {
                bool success = ReadFile(pipeIn_, buffer, DEFAULT_BUFFER_SIZE, &bytesRead, nullptr);
                if (! success) {
                    // TODO are there any other situations in which we should continue? 
                    break;
                }
                receive(buffer, bytesRead);
            }
            delete [] buffer;
        }};

        wait_ = std::thread{[this](){
            WaitForSingleObject(pInfo_.hProcess, INFINITE);
            helpers::ExitCode ec;
    		OSCHECK(GetExitCodeProcess(pInfo_.hProcess, &ec) != 0);
            terminated(ec);
        }};

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

#elif (defined ARCH_UNIX)

    LocalPTY::LocalPTY(Client * client, helpers::Command const & command):
        PTY{client},
        command_{command} {
        start();

    }

    LocalPTY::LocalPTY(Client * client, helpers::Command const & command, helpers::Environment const & env):
        PTY{client},
        command_{command},
        environment_{env} {
        start();
    }

    LocalPTY::~LocalPTY() {
        terminate();
        reader_.join();
        wait_.join();
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

        reader_ = std::thread{[this](){
            // create the buffer, and read while we can 
            char * buffer = new char[DEFAULT_BUFFER_SIZE];
            int cnt = 0;
            while (true) {
                cnt = ::read(pipe_, (void*)buffer, DEFAULT_BUFFER_SIZE);
                if (cnt == -1) {
                    if (errno == EINTR || errno == EAGAIN)
                        continue;
                    // the pipe is broken, which is direct action of the process terminating, exit the reader thread
                    break;
                }
                receive(buffer, cnt);
            } 
            delete [] buffer;
        }};

        wait_ = std::thread{[this](){
            helpers::ExitCode ec;
            pid_t x = waitpid(pid_, &ec, 0);
            ec = WEXITSTATUS(ec);
            // it is ok to see errno ECHILD, happens when process has already been terminated
            if (x < 0 && errno != ECHILD) 
                NOT_IMPLEMENTED; // error
            terminated(ec);
        }};
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



#endif

}

#ifdef HAHA

namespace tpp {

#if (defined ARCH_WINDOWS)

    LocalPTY::LocalPTY(helpers::Command const& command):
        command_{command},
		startupInfo_{},
   		pipeIn_{ INVALID_HANDLE_VALUE },
		pipeOut_{ INVALID_HANDLE_VALUE },
		active_{0} {
        start();
    }

	LocalPTY::LocalPTY(helpers::Command const& command, helpers::Environment const & env) :
		command_(command),
		environment_(env),
		startupInfo_{},
		pipeIn_{ INVALID_HANDLE_VALUE },
		pipeOut_{ INVALID_HANDLE_VALUE },
		active_{0} {
        start();
    }

    LocalPTY::~LocalPTY() {
        terminate();
		// now wait for all waiting threads to finish
		std::unique_lock<std::mutex> g(m_);
		while (active_ != 0)
		    cv_.wait(g);
        CloseHandle(pInfo_.hProcess);
        CloseHandle(pInfo_.hThread);
		ClosePseudoConsole(conPTY_);
		CloseHandle(pipeIn_);
        CloseHandle(pipeOut_);
		delete [] reinterpret_cast<char*>(startupInfo_.lpAttributeList);
    }

    // PTY interface

    void LocalPTY::send(char const * buffer, size_t bufferSize) {
		DWORD bytesWritten = 0;
		size_t start = 0;
		size_t i = 0;
		while (i < bufferSize) {
			if (buffer[i] == '`') {
				WriteFile(pipeOut_, buffer + start, static_cast<DWORD>(i + 1 - start), &bytesWritten, nullptr);
				start = i;
			}
			++i;
		}
		WriteFile(pipeOut_, buffer + start, static_cast<DWORD>(i - start), &bytesWritten, nullptr);
    }

    size_t LocalPTY::receive(char * buffer, size_t bufferSize) {
		DWORD bytesRead = 0;
		ReadFile(pipeIn_, buffer, static_cast<DWORD>(bufferSize), &bytesRead, nullptr);
		return bytesRead;
    }

    void LocalPTY::terminate() {
		helpers::ResourceGrabber g(active_, cv_);
        if (TerminateProcess(pInfo_.hProcess, std::numeric_limits<unsigned>::max()) != ERROR_SUCCESS) {
            // it could be that the process has already terminated
    		helpers::ExitCode ec = STILL_ACTIVE;
	    	GetExitCodeProcess(pInfo_.hProcess, &ec);		
			// the process has been terminated already, it's ok, otherwise throw last error (this could in theory be error from the GetExitCodeProcess, but that's ok for now)
            OSCHECK(ec != STILL_ACTIVE);
        }
    }

    helpers::ExitCode LocalPTY::waitFor() {
		helpers::ResourceGrabber g(active_, cv_);
		OSCHECK(WaitForSingleObject(pInfo_.hProcess, INFINITE) != WAIT_FAILED);
		helpers::ExitCode ec;
		OSCHECK(GetExitCodeProcess(pInfo_.hProcess, &ec) != 0);
        return ec;
    }

    void LocalPTY::resize(int cols, int rows) {
		// resize the underlying ConPTY
		COORD size;
		size.X = cols & 0xffff;
		size.Y = rows & 0xffff;
		ResizePseudoConsole(conPTY_, size);
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
		size_t attrListSize = 0;
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
    }

#elif (defined ARCH_UNIX)

    LocalPTY::LocalPTY(helpers::Command const& command):
        command_{command} {
        start();
    }

	LocalPTY::LocalPTY(helpers::Command const& command, helpers::Environment const & env) :
		command_(command),
		environment_(env) {
        start();
    }

    LocalPTY::~LocalPTY() {
        terminate();
    }

    // PTY interface

    void LocalPTY::send(char const * buffer, size_t bufferSize) {
		int nw = ::write(pipe_, (void*)buffer, bufferSize);
		// TODO check errors properly 
		ASSERT(nw >= 0 && static_cast<unsigned>(nw) == bufferSize);
    }

    size_t LocalPTY::receive(char * buffer, size_t bufferSize) {
		int cnt = 0;
		while (true) {
			cnt = ::read(pipe_, (void*)buffer, bufferSize);
			if (cnt == -1) {
				if (errno == EINTR || errno == EAGAIN)
					continue;
				// the pipe is broken, which is direct action of the process terminating, return eof
				return 0;
			}
			break;
		}
		return cnt;
    }

    void LocalPTY::terminate() {
		kill(pid_, SIGKILL);
    }

    helpers::ExitCode LocalPTY::waitFor() {
		helpers::ExitCode ec;
		pid_t x = waitpid(pid_, &ec, 0);
		ec = WEXITSTATUS(ec);
        // it is ok to see errno ECHILD, happens when process has already been terminated
		if (x < 0 && errno != ECHILD) 
			NOT_IMPLEMENTED; // error
		return ec;
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
    }

#endif


} // namespace vterm

#endif