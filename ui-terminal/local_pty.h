#pragma once
#if (defined ARCH_WINDOWS)
    #include "windows.h"
#elif (defined ARCH_UNIX)
    #include "sys/types.h"
#endif

#include "pty.h"

namespace ui {

    class LocalPTY : public PTY {
    public:
        LocalPTY(Client * client, helpers::Command const & command);
        LocalPTY(Client * client, helpers::Command const & command, helpers::Environment const & env);
        ~LocalPTY() override;

        void terminate() override;

    protected:
        void start();

        void resize(int cols, int rows) override;
        void send(char const * buffer, size_t bufferSize) override;

    private: 

        std::thread reader_;
        std::thread wait_;

        helpers::Command command_;
        helpers::Environment environment_;

#if (defined ARCH_WINDOWS)

#elif (defined ARCH_UNIX)

        /* Pipe to the process. */
		int pipe_;

        /* Pid of the process. */
		pid_t pid_;
#endif

    }; 

} // namespace ui

#ifdef HAHA

namespace tpp {

    class LocalPTY : public ui::Terminal::PTY {
    public:
        LocalPTY(helpers::Command const & command);
		LocalPTY(helpers::Command const& command, helpers::Environment const& env);

        ~LocalPTY();

        // PTY interface

        void send(char const * buffer, size_t bufferSize) override;

        size_t receive(char * buffer, size_t bufferSize) override;

        void terminate() override;

        helpers::ExitCode waitFor() override;

        void resize(int cols, int rows) override;

    private:

        helpers::Command command_;
		helpers::Environment environment_;

        /** Starts the command. 
         */
        void start();

#if (defined ARCH_WINDOWS)

        /* Startupo info which must be alive throughout the execution of the process.
         */
        STARTUPINFOEX startupInfo_;

		/* Handle to the ConPTY object created for the command. */
		HPCON conPTY_;

		/* The pipe from which input should be read. */
		HANDLE pipeIn_;

		/* Pipe to which data for the application should be sent. */
		HANDLE pipeOut_;

		/* Information about the process being executed. */
		PROCESS_INFORMATION pInfo_;

        /* Synchronization help so that terminate and waitFor() calls will not error if another thread runs destructor. */
        std::mutex m_;
        std::condition_variable cv_;
        std::atomic<unsigned> active_;
#elif (defined ARCH_UNIX)

        /* Pipe to the process. */
		int pipe_;

        /* Pid of the process. */
		pid_t pid_;
#endif


    }; // LocalPTY



} // namespace vterm

#endif