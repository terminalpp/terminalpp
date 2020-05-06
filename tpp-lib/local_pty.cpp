#include "local_pty.h"

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
        // then close all handles and the PTY, which interrupts the reader thread
		CloseHandle(pInfo_.hProcess);
		CloseHandle(pInfo_.hThread);
		ClosePseudoConsole(conPTY_);
		CloseHandle(pipeIn_);
		CloseHandle(pipeOut_);
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


#endif

} // namespace tpp