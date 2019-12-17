#pragma once
#if (defined ARCH_WINDOWS)

#include <mutex>
#include <condition_variable>
#include <atomic>

#include "windows.h"

#include "terminal.h"

namespace tpp {

    /** Virtual terminal using WSL instead of ConPTY.
     
     */
    class BypassPTY : public ui::Terminal::PTY {
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

		/* The pipe from which input should be read. */
		HANDLE pipeIn_;

		/* Pipe to which data for the application should be sent */
		HANDLE pipeOut_;

		/* Information about the process being executed. */
		PROCESS_INFORMATION pInfo_;

        /* Synchronization help so that terminate and waitFor() calls will not error if another thread runs destructor. */
        std::mutex m_;
        std::condition_variable cv_;
        std::atomic<unsigned> active_;

    }; // BypassPTY

} // namespace vterm
#endif