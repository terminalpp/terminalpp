#pragma once
#if (defined ARCH_UNIX)
#include <termios.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <thread>
#endif


#include "helpers/helpers.h"
#include "helpers/process.h"
#include "helpers/log.h"
#include "helpers/char.h"
#include "terminal.h"
#include "sequence.h"




namespace tpp {

    using Char = helpers::Char;

    class TerminalClient {
    public:

        TerminalClient(Terminal & terminal):
            terminal_{terminal},
            insideTmux_{InsideTMUX()} {
        }

        /** Returns true if the terminal client seems to be attached to the tmux terminal multipler. 
         */
        static bool InsideTMUX() {
            return helpers::Environment::Get("TMUX") != nullptr;
        }



    protected:

        /** Sends given buffer using the attached terminal. 
         */
        void send(char const * buffer, size_t numBytes);

        /** Sends given t++ sequence. 
         */
        void send(Sequence const & seq);

        /** Receives input from the attached terminal. 
         */
        size_t receive(char * buffer, size_t bufferSize, bool & success) {
            return terminal_.receive(buffer, bufferSize, success);
        }

        Terminal & terminal_;
        bool insideTmux_;
    }; // tpp::TerminalClient

    /** A client-side abstraction over an PTY. 

        - non tppSequences
        - tpp sequences
     */
    class TerminalClient {
    public:

        virtual ~TerminalClient() {
        }


    protected:

        virtual void start() {

        }

        /** Called when the input stream reaches its end. 
         
            No further data will be received after this method is called. Contains the unprocessed buffer as argument. 
         */
        virtual void inputEof(char const * buffer, char const * bufferEnd) {
            MARK_AS_UNUSED(buffer);
            MARK_AS_UNUSED(bufferEnd);
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
        StdTerminalClient(int in = STDIN_FILENO, int out = STDOUT_FILENO):
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

        void start() override {
            TerminalClient::start();
            reader_ = std::thread{[this](){
                readerThread();
            }};
        }

        ~StdTerminalClient() override {
            tcsetattr(in_, TCSAFLUSH, & backup_);
        }

        /** Returns true if the terminal client seems to be attached to the tmux terminal multipler. 
         */
        static bool InsideTMUX() {
            return helpers::Environment::Get("TMUX") != nullptr;
        }

        static constexpr size_t DEFAULT_BUFFER_SIZE = 1024;

    protected:
        void send(char const * buffer, size_t numBytes) override;
        
        void send(Sequence const & seq) override {
            if (insideTmux_)
                ::write(out_, "\033Ptmux;", 7);
            // TODO send the sequence
            if (insideTmux_)
                ::write(out_, "\033\\", 2);
        }

        /** Blocking read from the input file. 
         */
        size_t receive(char * buffer, size_t bufferSize, bool & success);

    private:

        void readerThread();

        char * parseTerminalInput(char * buffer, char const * bufferEnd);

        /** Finds the beginniong of a tpp sequence, or its prefix in the buffer. 
         
            Returns the beginning of the tpp sequence `"\033P+"`, or if the buffer terminates before the full sequence was read the beginning of possible tpp sequence start. 

            If not found, returns the bufferEnd. 
         */
        char * findTppStartPrefix(char * buffer, char const * bufferEnd);

        /** Given a start of the tpp sequence ("\033P+") or its prefix, calculates the range for the sequence's payload. 
         
            If the sequence is invalid, returns `(nullptr, nullptr)`. If the sequence seems valid, but the buffer does not conatin enough data, returns `(bufferEnd, bufferEnd)`. In other cases returns an std::pair where the first value is the first valid tpp sequence character and the second value is the sequence terminator. 
         */
        std::pair<char *, char*> findTppRange(char * tppStart, char const * bufferEnd);

        void parseTppSequence(char * buffer, char const * bufferEnd) {

        }

        int in_;
        int out_;
        bool insideTmux_;
        termios backup_;

        std::thread reader_;

    }; // tpp::StdTerminalClient
#endif




} // namespace tpp