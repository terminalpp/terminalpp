#include "bypass_pty.h"

namespace vterm {

#ifdef _WIN64

	/** Starts the local pseudoterminal for given command.
	 */
	BypassPTY::BypassPTY(helpers::Command const& command):
	    command_(command), 
		startupInfo_{},
		pipeIn_{ INVALID_HANDLE_VALUE },
		pipePTYOut_{ INVALID_HANDLE_VALUE },
		pipeOut_{ INVALID_HANDLE_VALUE },
		pipePTYIn_{ INVALID_HANDLE_VALUE } {
		start();
		monitor();
	}

	BypassPTY::~BypassPTY() {
		terminate();
	}

	size_t BypassPTY::sendData(char const* buffer, size_t size) {
		DWORD bytesWritten = 0;
		WriteFile(pipeOut_, buffer, static_cast<DWORD>(size), &bytesWritten, nullptr);
		// TODO check this properly for errors
		ASSERT(bytesWritten == size);
		return bytesWritten;
	}

	size_t BypassPTY::receiveData(char* buffer, size_t availableSize) {
		DWORD bytesRead = 0;
		bool readOk = ReadFile(pipeIn_, buffer, static_cast<DWORD>(availableSize), &bytesRead, nullptr);
		// make sure that if readOk is false nothing was read
		ASSERT(readOk || bytesRead == 0);
		return bytesRead;
	}

	void BypassPTY::resize(unsigned cols, unsigned rows) {
		std::string s = STR("`r" << cols << ':' << rows << ';');
		sendData(s.c_str(), s.size());
	}

	void BypassPTY::doTerminate() {
		TerminateProcess(pInfo_.hProcess, -1);
	}

	helpers::ExitCode BypassPTY::doWaitFor() {
		size_t result = WaitForSingleObject(pInfo_.hProcess, INFINITE);
		helpers::ExitCode ec;
		GetExitCodeProcess(pInfo_.hProcess, &ec);
		// we must close the handles so that any pending reads will be interrupted
		CloseHandle(pInfo_.hProcess);
		CloseHandle(pInfo_.hThread);
		if (pipeIn_ != INVALID_HANDLE_VALUE)
			CloseHandle(pipeIn_);
		if (pipeOut_ != INVALID_HANDLE_VALUE)
			CloseHandle(pipeOut_);
		return ec;
	}

	void BypassPTY::start() {
		// create the pipes
		SECURITY_ATTRIBUTES attrs;
		attrs.nLength = sizeof(SECURITY_ATTRIBUTES);
		attrs.bInheritHandle = TRUE;
		attrs.lpSecurityDescriptor = NULL;
		// first create the pipes we need, no security arguments and we use default buffer size for now
		OSCHECK(
			CreatePipe(&pipePTYIn_, &pipeOut_, &attrs, 0) && CreatePipe(&pipeIn_, &pipePTYOut_, &attrs, 0)
		) << "Unable to create pipes for the subprocess";
		// make sure that own handles are not inherited
		OSCHECK(
			SetHandleInformation(pipeIn_, HANDLE_FLAG_INHERIT, 0) && SetHandleInformation(pipeOut_, HANDLE_FLAG_INHERIT, 0)
		) << "Unable to disable child process handle inheritance";
		// and now create the process
		STARTUPINFO sInfo;
		ZeroMemory(&sInfo, sizeof(STARTUPINFO));
		sInfo.cb = sizeof(STARTUPINFO);
		sInfo.hStdError = pipePTYOut_;
		sInfo.hStdOutput = pipePTYOut_;
		sInfo.hStdInput = pipePTYIn_;
		sInfo.dwFlags |= STARTF_USESTDHANDLES;
		std::string cmd = command_.toString();
		OSCHECK(CreateProcess(NULL,
			&cmd[0], // the command to execute
			NULL, // process security attributes 
			NULL, // primary thread security attributes 
			true, // handles are inherited 
			0, // creation flags 
			NULL, // use parent's environment 
			NULL, // use parent's directory 
			&sInfo,  // startup info
			&pInfo_));  // info about the process
		// we can close our handles to the other ends now
		CloseHandle(pipePTYOut_);
		CloseHandle(pipePTYIn_);
	}

#elif __linux__
	BypassPTY::BypassPTY(helpers::Command const& command) :
		command_(command) {
		NOT_IMPLEMENTED;
	}

	BypassPTY::~BypassPTY() {
		terminate();
	}

	size_t BypassPTY::sendData(char const* buffer, size_t size) {
		NOT_IMPLEMENTED;
	}

	size_t BypassPTY::receiveData(char* buffer, size_t availableSize) {
		NOT_IMPLEMENTED;
	}

	void BypassPTY::resize(unsigned cols, unsigned rows) {
		NOT_IMPLEMENTED;
	}

	void BypassPTY::doTerminate() {
		NOT_IMPLEMENTED;
	}

	helpers::ExitCode BypassPTY::doWaitFor() {
		NOT_IMPLEMENTED;
	}

#endif

} // namespace vterm