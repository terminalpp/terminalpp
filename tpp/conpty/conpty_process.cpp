#include <thread>

#include "conpty_process.h"
#include "../tpp.h"

namespace tpp {

	ConPTYProcess::ConPTYProcess(std::string const & command, vterm::VTerm * terminal) :
		command_{command},
		startupInfo_{},
		conPTY_{INVALID_HANDLE_VALUE},
		pipeIn_{INVALID_HANDLE_VALUE},
		pipeOut_{INVALID_HANDLE_VALUE} {
		terminal_ = terminal;
		startupInfo_.lpAttributeList = nullptr; // just to be sure
		createPseudoConsole();
		std::thread t(ConPTYProcess::InputPipeReader, this);
		t.detach();
		execute();
	}

	void ConPTYProcess::createPseudoConsole() {
		HRESULT result{ E_UNEXPECTED };
		HANDLE pipePTYIn{ INVALID_HANDLE_VALUE };
		HANDLE pipePTYOut{ INVALID_HANDLE_VALUE };
		// first create the pipes we need, no security arguments and we use default buffer size for now
		if (!CreatePipe(&pipePTYIn, &pipeOut_, nullptr, 0) || !CreatePipe(&pipeIn_, &pipePTYOut, nullptr, 0))
			THROW(Win32Error("Unable to create pipes for the subprocess"));
		// determine the console size from the terminal we have
		COORD consoleSize{};
 		consoleSize.X = terminal() != nullptr ? terminal()->cols() : 80;
		consoleSize.Y = terminal() != nullptr ? terminal()->rows() : 25;
		// now create the pseudo console
		result = CreatePseudoConsole(consoleSize, pipePTYIn, pipePTYOut, 0, &conPTY_);
		// delete the pipes on PTYs end, since they are now in conhost and will be deleted when the conpty is deleted
		if (pipePTYIn != INVALID_HANDLE_VALUE)
			CloseHandle(pipePTYIn);
		if (pipePTYOut != INVALID_HANDLE_VALUE)
			CloseHandle(pipePTYOut);
		if (result != S_OK)
			THROW(Win32Error("Unable to open pseudo console"));
	}

	void ConPTYProcess::execute() {
		// first generate the startup info 
		STARTUPINFOEX startupInfo{};
		size_t attrListSize = 0;
		startupInfo.StartupInfo.cb = sizeof(STARTUPINFOEX);
		// allocate the attribute list of required size
		InitializeProcThreadAttributeList(nullptr, 1, 0, &attrListSize); // get size of list of 1 attribute
		startupInfo.lpAttributeList = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(malloc(attrListSize));
		// initialize the attribute list
		if (!InitializeProcThreadAttributeList(startupInfo.lpAttributeList, 1, 0, &attrListSize))
			THROW(Win32Error("Unable to create attribute list"));
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
			THROW(Win32Error("Unable to set pseudoconsole attribute"));
		// finally, create the process with given commandline
		if (! CreateProcess(
			nullptr,
			& command_[0], // the command to execute
			nullptr, // process handle cannot be inherited
			nullptr, // thread handle cannot be inherited
			false, // the new process does not inherit any handles
			EXTENDED_STARTUPINFO_PRESENT, // we have extra info 
			nullptr, // use parent's environment
			nullptr, // use parent's directory
			&startupInfo.StartupInfo, // startup info
			&pInfo_ // info about the process
		))
			THROW(Win32Error(STR("Unable to start process " << command_)));
	}

	void ConPTYProcess::InputPipeReader(ConPTYProcess * p) {
		char * buffer;
		size_t size;
		bool readOk;
		size_t bytesRead;
		do {
			buffer = p->getInputBuffer(size);
			readOk = ReadFile(p->pipeIn_, buffer, size, &bytesRead, nullptr);
			p->commitInputBuffer(buffer, bytesRead);
		} while (readOk);
	}
} // namespace tpp
