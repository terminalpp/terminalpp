#pragma once

#if (defined ARCH_UNIX)
#include <termios.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#endif

#include "helpers/helpers.h"
#include "helpers/process.h"

#include "sequence.h"

namespace tpp {

    class TerminalPTY {
    public:
        /** Sends a buffer. 
         */
        virtual void send(char const * buffer, size_t numBytes) = 0;

        virtual void send(Sequence const & seq) {
            send("\033P+", 3);
            seq.sendTo(*this);
            send("\007", 1);
        }

        /** Receives data from the input (blocking) 
         */
        virtual size_t receive(char * buffer, size_t bufferSize, bool & success) = 0;
    };

#if (defined ARCH_UNIX)

    /** Simple terminal that connects to standard in and out files. 
     */
    class StdPTY : public TerminalPTY {
    public:
        StdPTY(int in = STDIN_FILENO, int out = STDOUT_FILENO):
            in_{in},
            out_{out},
            insideTmux_{InsideTMUX()} {
            tcgetattr(in_, & backup_);
            termios raw = backup_;
            raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
            raw.c_oflag &= ~(OPOST);
            raw.c_cflag |= (CS8);
            raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
            tcsetattr(in_, TCSAFLUSH, & raw);
        }

        ~StdPTY() {
            tcsetattr(in_, TCSAFLUSH, & backup_);
        }

        void send(char const * buffer, size_t numBytes) override;

        void send(Sequence const & seq) override;

        /** Blocking read from the input file. 
         */
        size_t receive(char * buffer, size_t bufferSize, bool & success) override;

        /** Returns true if the terminal seems to be attached to the tmux terminal multipler. 
         */
        static bool InsideTMUX() {
            return helpers::Environment::Get("TMUX") != nullptr;
        }

    private:
        int in_;
        int out_;
        bool insideTmux_;
        termios backup_;

    }; // tpp::StdPTY

#endif // ARCH_UNIX


} // namespace tpp