#pragma once
#if (defined ARCH_WINDOWS)

#include "windows.h"

#include "pty.h"

namespace vterm {

    /** Virtual terminal using WSL instead of ConPTY.
     
     */
    class BypassPTY : public PTY {
    public:
        BypassPTY(helpers::Command const & command);

        ~BypassPTY();

        // PTY interface

        void send(char const * buffer, size_t bufferSize) override;

        size_t receive(char * buffer, size_t bufferSize) override;

        void terminate() override;

        helpers::ExitCode waitFor() override;

        void resize(int cols, int rows) override;

    private:
        helpers::Command command_;

		/* The pipe from which input should be read and output pipe handle for the process. */
		HANDLE pipeIn_;
		HANDLE pipePTYOut_;

		/* Pipe to which data for the application should be sent and input pipe handle for the process. */
		HANDLE pipeOut_;
		HANDLE pipePTYIn_;

		/* Information about the process being executed. */
		PROCESS_INFORMATION pInfo_;

    }; // BypassPTY

} // namespace vterm
#endif