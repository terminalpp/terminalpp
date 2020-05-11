#pragma once

#if (defined ARCH_UNIX)
#include <termios.h>
#include <pthread.h>
#include <mutex>
#include <atomic>
#endif

#include <thread>

#include "pty.h"

namespace tpp {

    class LocalPTYMaster : public PTYMaster {
    public:

        LocalPTYMaster(helpers::Command const & command);
        LocalPTYMaster(helpers::Command const & command, helpers::Environment const & env);
        ~LocalPTYMaster() override;

        void terminate() override;
        void send(char const * buffer, size_t numBytes) override;
        size_t receive(char * buffer, size_t bufferSize) override;
        void resize(int cols, int rows) override;

    private:

        void start();

        helpers::Command command_;
        helpers::Environment environment_;

        std::thread waiter_;

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

#elif (defined ARCH_UNIX)

        /* Pipe to the process. */
		int pipe_;

        /* Pid of the process. */
		pid_t pid_;
#endif

    }; // tpp::LocalPTYMaster



#if (defined ARCH_UNIX)
    class LocalPTYSlave : public PTYSlave {
    public:

        LocalPTYSlave();
        ~LocalPTYSlave() override;

        std::pair<int, int> size() const override;

        void send(char const * buffer, size_t numBytes) override;

        void send(Sequence const & seq) override;
        
        size_t receive(char * buffer, size_t bufferSize) override;

        /** Returns true if the terminal seems to be attached to the tmux terminal multipler. 
         */
        static bool InsideTMUX() {
            return helpers::Environment::Get("TMUX") != nullptr;
        }

    private:

        static constexpr int IDLE = 0;
        static constexpr int RECEIVING = 1;
        static constexpr int DESTROYING = 2;

        bool insideTmux_;
        termios backup_;

        static pthread_t volatile ReaderThread_;
        static std::atomic<bool> Receiving_;
        static LocalPTYSlave * volatile Slave_;
        static void SIGWINCH_handler(int signo);
        static void SIGINT_handler(int signo);

    }; // tpp::LocalPTYSlave
#endif

}

