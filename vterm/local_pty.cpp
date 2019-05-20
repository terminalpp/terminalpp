#ifdef WIN32
#include "helpers/win32.h"
#endif

#include "local_pty.h"

namespace vterm {

#ifdef WIN32

	LocalPTY::LocalPTY(std::string const & command) :
		command_{ command },
		startupInfo_{},
		conPTY_{ INVALID_HANDLE_VALUE },
		pipeIn_{ INVALID_HANDLE_VALUE },
		pipeOut_{ INVALID_HANDLE_VALUE } {
		startupInfo_.lpAttributeList = nullptr; // just to be sure
		createPseudoConsole();
		start();
	}

	LocalPTY::~LocalPTY() {
		if (conPTY_ != INVALID_HANDLE_VALUE)
			ClosePseudoConsole(conPTY_);
		if (pipeIn_ != INVALID_HANDLE_VALUE)
			CloseHandle(pipeIn_);
		if (pipeOut_ != INVALID_HANDLE_VALUE)
			CloseHandle(pipeOut_);
		free(startupInfo_.lpAttributeList);
	}

	size_t LocalPTY::sendData(char const * buffer, size_t size) {
		DWORD bytesWritten = 0;
		WriteFile(pipeOut_, buffer, static_cast<DWORD>(size), &bytesWritten, nullptr);
		// TODO check this properly for errors
		ASSERT(bytesWritten == size);
		return bytesWritten;
	}

	size_t LocalPTY::receiveData(char* buffer, size_t availableSize) {
		DWORD bytesRead = 0;
		bool readOk = ReadFile(pipeIn_, buffer, static_cast<DWORD>(availableSize), &bytesRead, nullptr);
		// make sure that if readOk is false nothing was read
		ASSERT(readOk || bytesRead == 0);
		return bytesRead;
	}

	void LocalPTY::resize(unsigned cols, unsigned rows) {
		// resize the underlying ConPTY
		COORD size;
		size.X = cols;
		size.Y = rows;
		ResizePseudoConsole(conPTY_, size);
	}

	void LocalPTY::createPseudoConsole() {
		HRESULT result{ E_UNEXPECTED };
		HANDLE pipePTYIn{ INVALID_HANDLE_VALUE };
		HANDLE pipePTYOut{ INVALID_HANDLE_VALUE };
		// first create the pipes we need, no security arguments and we use default buffer size for now
		if (!CreatePipe(&pipePTYIn, &pipeOut_, nullptr, 0) || !CreatePipe(&pipeIn_, &pipePTYOut, nullptr, 0))
			THROW(helpers::Win32Error("Unable to create pipes for the subprocess"));
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
		if (result != S_OK)
			THROW(helpers::Win32Error("Unable to open pseudo console"));
	}

	void LocalPTY::start() {
		// first generate the startup info 
		STARTUPINFOEX startupInfo{};
		size_t attrListSize = 0;
		startupInfo.StartupInfo.cb = sizeof(STARTUPINFOEX);
		// allocate the attribute list of required size
		InitializeProcThreadAttributeList(nullptr, 1, 0, &attrListSize); // get size of list of 1 attribute
		startupInfo.lpAttributeList = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(malloc(attrListSize));
		// initialize the attribute list
		if (!InitializeProcThreadAttributeList(startupInfo.lpAttributeList, 1, 0, &attrListSize))
			THROW(helpers::Win32Error("Unable to create attribute list"));
		// set the pseudoconsole attribute
		if (!UpdateProcThreadAttribute(
			startupInfo.lpAttributeList,
			0,
			PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE,
			conPTY_,
			sizeof(HPCON),
			nullptr,
			nullptr
		))
			THROW(helpers::Win32Error("Unable to set pseudoconsole attribute"));
		// finally, create the process with given commandline
		if (!CreateProcess(
			nullptr,
			&command_[0], // the command to execute
			nullptr, // process handle cannot be inherited
			nullptr, // thread handle cannot be inherited
			false, // the new process does not inherit any handles
			EXTENDED_STARTUPINFO_PRESENT, // we have extra info 
			nullptr, // use parent's environment
			nullptr, // use parent's directory
			&startupInfo.StartupInfo, // startup info
			&pInfo_ // info about the process
		))
			THROW(helpers::Win32Error(STR("Unable to start process " << command_)));
	}

#endif




} // namespace vterm