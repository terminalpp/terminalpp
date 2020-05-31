
#if (defined ARCH_WINDOWS)
#include "helpers/string.h"
#include "helpers/locks.h"

#include "bypass_pty.h"

namespace tpp {

    BypassPTYMaster::BypassPTYMaster(Command const & command):
        command_{command},
		pipeIn_{ INVALID_HANDLE_VALUE },
		pipeOut_{ INVALID_HANDLE_VALUE } {
        start();
    }

    BypassPTYMaster::~BypassPTYMaster() {
        terminate();
        waiter_.join();
    }

    void BypassPTYMaster::terminate() {
        if (TerminateProcess(pInfo_.hProcess, std::numeric_limits<unsigned>::max()) != ERROR_SUCCESS) {
            // it could be that the process has already terminated
    		ExitCode ec = STILL_ACTIVE;
	    	GetExitCodeProcess(pInfo_.hProcess, &ec);
			// the process has been terminated already, it's ok, otherwise throw last error (this could in theory be error from the GetExitCodeProcess, but that's ok for now)
            OSCHECK(ec != STILL_ACTIVE);
        }
    }

    void BypassPTYMaster::start() {
		//  input and output handles for the process
		HANDLE pipePTYOut;
		HANDLE pipePTYIn;
		// create the pipes
		SECURITY_ATTRIBUTES attrs;
		attrs.nLength = sizeof(SECURITY_ATTRIBUTES);
		attrs.bInheritHandle = TRUE;
		attrs.lpSecurityDescriptor = NULL;
		// first create the pipes we need, no security arguments and we use default buffer size for now
		OSCHECK(
			CreatePipe(&pipePTYIn, &pipeOut_, &attrs, 0) && CreatePipe(&pipeIn_, &pipePTYOut, &attrs, 0)
		) << "Unable to create pipes for the subprocess";
		// make sure that own handles are not inherited
		OSCHECK(
			SetHandleInformation(pipeIn_, HANDLE_FLAG_INHERIT, 0) && SetHandleInformation(pipeOut_, HANDLE_FLAG_INHERIT, 0)
		) << "Unable to disable child process handle inheritance";
		// and now create the process
		STARTUPINFO sInfo;
		ZeroMemory(&sInfo, sizeof(STARTUPINFO));
		sInfo.cb = sizeof(STARTUPINFO);
		sInfo.hStdError = pipePTYOut;
		sInfo.hStdOutput = pipePTYOut;
		sInfo.hStdInput = pipePTYIn;
		sInfo.dwFlags |= STARTF_USESTDHANDLES;
		utf16_string cmd = UTF8toUTF16(command_.toString());
		OSCHECK(CreateProcess(NULL,
			&cmd[0], // the command to execute
			NULL, // process security attributes 
			NULL, // primary thread security attributes 
			true, // handles are inherited 
			0, // creation flags 
			NULL, // use parent's environment 
			NULL, // use parent's directory 
			&sInfo,  // startup info
			&pInfo_)  // info about the process
		) << "Unable to execute process " << command_.toString();
		// we can close our handles to the other ends now
		OSCHECK(CloseHandle(pipePTYOut));
		OSCHECK(CloseHandle(pipePTYIn));
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
            CloseHandle(pipeIn_);
            CloseHandle(pipeOut_);
        }};
    }

    void BypassPTYMaster::resize(int cols, int rows) {
		DWORD bytesWritten = 0;
		std::string s = STR("`r" << cols << ':' << rows << ';');
		WriteFile(pipeOut_, s.c_str(), static_cast<DWORD>(s.size()), &bytesWritten, nullptr);
		// TODO check properly how stuff was written and error in an appropriate way
    }

    void BypassPTYMaster::send(char const * buffer, size_t bufferSize) {
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

    size_t BypassPTYMaster::receive(char * buffer, size_t bufferSize) {
        DWORD bytesRead = 0;
        ReadFile(pipeIn_, buffer, static_cast<DWORD>(bufferSize), &bytesRead, nullptr);
        return bytesRead;
    }

} // namespace ui 

#endif