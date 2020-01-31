#pragma once
#if (defined ARCH_WINDOWS) 

#include <iostream>
#include <windows.h>

#include "helpers/helpers.h"

namespace tpp {

    inline void AttachConsole() {
    	if (::AttachConsole(ATTACH_PARENT_PROCESS) == 0) {
			if (GetLastError() != ERROR_INVALID_HANDLE)
			  OSCHECK(false) << "Error when attaching to parent process console";
			// create the console,
		    OSCHECK(AllocConsole()) << "No parent process console and cannot allocate one";
#ifdef NDEBUG
			// and hide the window immediately if we are in release mode
			ShowWindow(GetConsoleWindow(), SW_HIDE);
#endif
		}
        // this is ok, console cannot be detached, so we are fine with keeping the file handles forewer,
        // nor do we need to FreeConsole at any point
        FILE* fpstdin = stdin, * fpstdout = stdout, * fpstderr = stderr;
        // patch the cin, cout, cerr
        OSCHECK(freopen_s(&fpstdin, "CONIN$", "r", stdin) == 0);
        OSCHECK(freopen_s(&fpstdout, "CONOUT$", "w", stdout) == 0);
        OSCHECK(freopen_s(&fpstderr, "CONOUT$", "w", stderr) == 0);
        std::cin.clear();
        std::cout.clear();
        std::cerr.clear();
    }

} // namespace tpp

#endif