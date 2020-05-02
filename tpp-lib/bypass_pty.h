#pragma once
#if (defined ARCH_WINDOWS)

#include <mutex>
#include <condition_variable>
#include <atomic>
#include <thread>

#include "windows.h"

#include "ui-terminal/pty.h"

namespace tpp {

    using ui::PTY;
    using ui::IOPTY;

    /** A big mess of thread unsafe stuff for now, but the light at the end of the tunnel is visible...
     */
    class BypassPTY : public IOPTY {
    public:

        BypassPTY(Client * client, helpers::Command const & command);

        ~BypassPTY() override;

        void terminate() override;

    protected:

        void start() override ;

        void resize(int cols, int rows) override;
        void send(char const * buffer, size_t bufferSize) override;
        size_t receive(char * buffer, size_t bufferSize, bool & success) override;
        helpers::ExitCode waitAndGetExitCode() override;

    private:

        helpers::Command command_;

		/* The pipe from which input should be read. */
		HANDLE pipeIn_;

		/* Pipe to which data for the application should be sent */
		HANDLE pipeOut_;

		/* Information about the process being executed. */
		PROCESS_INFORMATION pInfo_;

    }; // ui::BypassPTY

} // namespace ui

#endif