#pragma once

#include "sequences.h"

namespace tpp {

    /** A client-side abstraction over an PTY. 

        - non tppSequences
        - tpp sequences
     */
    class TerminalClient {
    public:

        virtual ~TerminalClient() {
        }

    protected:

        /** Called when the terminal the client is attached to closes from the server side. 
         
            Does nothing by default, but subclasses may override and decide to react.
         */
        virtual void closed() {
        }

        virtual void send(char const * buffer, size_t numBytes) = 0;

        virtual void send(Sequence const & seq) {
            
        }


        /** Called when normal input is received. 
         */
        virtual size_t processInput(char const * buffer, char const * bufferEnd) = 0;

        /** Called when a t++ sequence has been received. 
         */
        virtual void processTppSequence(Sequence seq) = 0;


    };

#if (defined ARCH_UNIX)

    class StdTerminalClient : public TerminalClient {
    public:
        StdTerminalClient(int in, int out):
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

        ~StdTerminalClient() override {
            tcsetattr(in_, TCSAFLUSH, & backup_);
        }

        /** Returns true if the terminal client seems to be attached to the tmux terminal multipler. 
         */
        static bool InsideTMUX() {
            return helpers::Environment::Get("TMUX") != nullptr;
        }

    protected:
        void send(char const * buffer, size_t numBytes) override {
            if (insideTmux_) {
                // we have to properly escape the buffer
                size_t start = 0;
                size_t end = 0;
                while (end < numBytes) {
                    if (buffer[end] == '\033') {
                        if (start != end)
                            ::write(out_, buffer + start, end - start);
                        ::write(out_, "\033\033", 2);
                        start = ++end;
                    } else {
                        ++end;
                    }
                }
                if (start != end)
                    ::write(out_, buffer+start, end - start);
            } else {
                ::write(out_, buffer, numBytes);
            }

        }

        void send(Sequence const & seq) override {
            if (insideTmux_)
                ::write(out_, "\033Ptmux;", 7);
            // TODO send the sequence
            if (insideTmux)
                ::write(out_, "\033\\", 2);
        }

    private:
        int in_;
        int out_;
        bool insideTmux_;

        std::thread reader_;

    }; // tpp::StdTerminalClient
#endif




} // namespace tpp