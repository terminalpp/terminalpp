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

#include "local_pty.h"

#include <iostream>

namespace tpp {

#if (defined ARCH_WINDOWS)    

    LocalPTYMaster::LocalPTYMaster(helpers::Command const & command):
        command_{command},
		startupInfo_{},
   		pipeIn_{ INVALID_HANDLE_VALUE },
		pipeOut_{ INVALID_HANDLE_VALUE } {
        start();
    }

    LocalPTYMaster::LocalPTYMaster(helpers::Command const & command, helpers::Environment const & env):
		command_(command),
		environment_(env),
		startupInfo_{},
		pipeIn_{ INVALID_HANDLE_VALUE },
		pipeOut_{ INVALID_HANDLE_VALUE } {
        start();
    }

    LocalPTYMaster::~LocalPTYMaster() {
        // first terminate the process and wait for it
        terminate();
		waiter_.join();
        // and free the rest of the resources
		delete [] reinterpret_cast<char*>(startupInfo_.lpAttributeList);
    }

    void LocalPTYMaster::terminate() {
        TerminateProcess(pInfo_.hProcess, std::numeric_limits<unsigned>::max());
    }

    void LocalPTYMaster::start() {
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
        // start the waiter thread
        waiter_ = std::thread{[this](){
            while (true) {
                OSCHECK(WaitForSingleObject(pInfo_.hProcess, INFINITE) == 0);
                OSCHECK(GetExitCodeProcess(pInfo_.hProcess, &exitCode_) != 0);
                if (exitCode_ != STILL_ACTIVE)
                    break;
            }
            terminated_.store(true);
            // then close all handles and the PTY, which interrupts the reader thread
            CloseHandle(pInfo_.hProcess);
            CloseHandle(pInfo_.hThread);
            ClosePseudoConsole(conPTY_);
            CloseHandle(pipeIn_);
            CloseHandle(pipeOut_);
        }};
    }

    void LocalPTYMaster::resize(int cols, int rows) {
		// resize the underlying ConPTY
		COORD size;
		size.X = cols & 0xffff;
		size.Y = rows & 0xffff;
		ResizePseudoConsole(conPTY_, size);
    }

    void LocalPTYMaster::send(char const * buffer, size_t bufferSize) {
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

    size_t LocalPTYMaster::receive(char * buffer, size_t bufferSize) {
        DWORD bytesRead = 0;
        ASSERT(static_cast<DWORD>(bufferSize) == bufferSize);
        ReadFile(pipeIn_, buffer, static_cast<DWORD>(bufferSize), &bytesRead, nullptr);
        return bytesRead;
    }

#elif (defined ARCH_UNIX)

    LocalPTYMaster::LocalPTYMaster(helpers::Command const & command):
        command_{command} {
        start();

    }

    LocalPTYMaster::LocalPTYMaster(helpers::Command const & command, helpers::Environment const & env):
        command_{command},
        environment_{env} {
        start();
    }

    LocalPTYMaster::~LocalPTYMaster() {
        terminate();
        waiter_.join();
    }

    void LocalPTYMaster::terminate() {
        kill(pid_, SIGKILL);
    }

    void LocalPTYMaster::start() {
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

        waiter_ = std::thread{[this](){
            pid_t x = waitpid(pid_, &exitCode_, 0);
            exitCode_ = WEXITSTATUS(exitCode_);
            // it is ok to see errno ECHILD, happens when process has already been terminated
            if (x < 0 && errno != ECHILD) 
                NOT_IMPLEMENTED; // error
            // mark as terminated
            terminated_.store(true);
        }};
    }

    void LocalPTYMaster::resize(int cols, int rows) {
        struct winsize s;
        s.ws_row = rows;
        s.ws_col = cols;
        s.ws_xpixel = 0;
        s.ws_ypixel = 0;
        if (ioctl(pipe_, TIOCSWINSZ, &s) < 0)
            NOT_IMPLEMENTED;
    }

    void LocalPTYMaster::send(char const * buffer, size_t bufferSize) {
		int nw = ::write(pipe_, (void*)buffer, bufferSize);
		// TODO check errors properly 
		ASSERT(nw >= 0 && static_cast<unsigned>(nw) == bufferSize);
    }

    size_t LocalPTYMaster::receive(char * buffer, size_t bufferSize) {
        while (true) {
            int cnt = 0;
            cnt = ::read(pipe_, (void*)buffer, bufferSize);
            if (cnt == -1) {
                if (errno == EINTR || errno == EAGAIN)
                    continue;
                return 0;
            } else {
                return static_cast<size_t>(cnt);
            }
        }
    }

#endif

    // LocalPTYSlave

#if (defined ARCH_UNIX)

    pthread_t volatile  LocalPTYSlave::ReaderThread_;
    std::atomic<bool> LocalPTYSlave::Receiving_{false};
    LocalPTYSlave * volatile LocalPTYSlave::Slave_ = nullptr;

    void LocalPTYSlave::SIGWINCH_handler(int signo) {
        if (Slave_ != nullptr) {
            ResizedEvent::Payload p{Slave_->size()};
            Slave_->onResized(p, Slave_);
        }
    }


    LocalPTYSlave::LocalPTYSlave():
        insideTmux_{InsideTMUX()} {
        OSCHECK(tcgetattr(STDIN_FILENO, & backup_) == 0);
        termios raw = backup_;
        raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
        raw.c_oflag &= ~(OPOST);
        raw.c_cflag |= (CS8);
        raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
        OSCHECK(tcsetattr(STDIN_FILENO, TCSAFLUSH, & raw) == 0);
        Slave_ = this;
        struct sigaction sa;
        sigemptyset(&sa.sa_mask);
        sa.sa_handler = SIGWINCH_handler;
        sa.sa_flags = 0;        
        OSCHECK(sigaction(SIGWINCH, &sa, nullptr) == 0);        
    }

    LocalPTYSlave::~LocalPTYSlave() {
        // start the destroy process
        Slave_ = nullptr;
        // TODO busy wait, ok now, with C++20 switch to std::atomic::wait?
        while (Receiving_ == true) {
            // send the signal to the reader thread
            pthread_kill(ReaderThread_, SIGWINCH);
            std::this_thread::yield();
        }
        struct sigaction sa;
        sigemptyset(&sa.sa_mask);
        sa.sa_handler = SIG_DFL;
        sa.sa_flags = 0;        
        sigaction(SIGWINCH, &sa, nullptr);        
        sa.sa_handler = SIG_DFL;
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &backup_);
    }

    std::pair<int, int> LocalPTYSlave::size() const {
        winsize size;
        OSCHECK(ioctl(STDOUT_FILENO, TIOCGWINSZ, &size) != -1);
        return std::pair<int,int>{size.ws_col, size.ws_row};
    }

    void LocalPTYSlave::send(char const * buffer, size_t numBytes) {
        if (insideTmux_) {
            // we have to properly escape the buffer
            size_t start = 0;
            size_t end = 0;
            while (end < numBytes) {
                if (buffer[end] == '\033') {
                    if (start != end)
                        ::write(STDOUT_FILENO, buffer + start, end - start);
                    ::write(STDOUT_FILENO, "\033\033", 2);
                    start = ++end;
                } else {
                    ++end;
                }
            }
            if (start != end)
                ::write(STDOUT_FILENO, buffer + start, end - start);
        } else {
            ::write(STDOUT_FILENO, buffer, numBytes);
        }
    }

    void LocalPTYSlave::send(Sequence const & seq) {
        if (insideTmux_)
            ::write(STDOUT_FILENO, "\033Ptmux;", 7);
        PTYSlave::send(seq);
        if (insideTmux_)
            ::write(STDOUT_FILENO, "\033\\", 2);
    }


    size_t LocalPTYSlave::receive(char * buffer, size_t bufferSize) {
            // if there is no slave, 
        if (Slave_ == nullptr)
            return false;
        ReaderThread_ = pthread_self(); 
        Receiving_.store(true);
        while (true) {
            int cnt = 0;
            cnt = ::read(STDIN_FILENO, (void*)buffer, bufferSize);
            if (cnt == -1) {
                if ((errno == EINTR || errno == EAGAIN) && Slave_ != nullptr)
                    continue;
                break;
            } else {
                Receiving_.store(false);
                return static_cast<size_t>(cnt);
            }
        }
        Receiving_.store(false);
        return 0;
    }

#endif

} // namespace tpp