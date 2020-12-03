#pragma once
#if (defined ARCH_WINDOWS)

#include <mutex>
#include <condition_variable>
#include <atomic>
#include <thread>

#include "windows.h"

#include "pty.h"

namespace tpp {

    class BypassPTYMaster : public PTYMaster {
    public:
        explicit BypassPTYMaster(Command const & command);
        ~BypassPTYMaster() override;

        void terminate() override;
        void send(char const * buffer, size_t numBytes) override;
        size_t receive(char * buffer, size_t bufferSize) override;
        void resize(int cols, int rows) override;

    private:

        void start();

        Command command_;

        std::thread waiter_;

		/* The pipe from which input should be read. */
		HANDLE pipeIn_;

		/* Pipe to which data for the application should be sent. */
		HANDLE pipeOut_;

		/* Information about the process being executed. */
		PROCESS_INFORMATION pInfo_;

    }; // tpp::BypassPTYMaster

} // namespace ui

#endif