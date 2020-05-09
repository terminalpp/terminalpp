#pragma once

#include <thread>

#include "helpers/helpers.h"
#include "helpers/process.h"
#include "helpers/log.h"
#include "helpers/char.h"
#include "pty.h"
#include "sequence.h"

namespace tpp {

    class TimeoutError : public helpers::Exception {
    }; 


    class TerminalClient {
    public:
        static constexpr size_t DEFAULT_BUFFER_SIZE = 1024;

        class Sync;

        virtual ~TerminalClient() {
            delete pty_;
            reader_.join();
            delete [] buffer_;
        }

    protected:

        TerminalClient(PTYSlave * pty);

        /** Called when normal input is received from the terminal. 
         
            The implementation should process the received input and return the number of bytes processed. These will be removed from the buffer, while any unprocessed data will be prepended to data received next. 
         */
        virtual size_t received(char const * buffer, char const * bufferEnd) = 0;

        /** Called when a t++ sequence has been received. 
         */
        virtual void receivedSequence(Sequence::Kind kind, char const * buffer, char const * bufferEnd) = 0;

        virtual void inputEof(char const * buffer, char const * bufferEnd) {
            MARK_AS_UNUSED(buffer);
            MARK_AS_UNUSED(bufferEnd);
        }

        virtual void resized(int cols, int rows) {
            MARK_AS_UNUSED(cols);
            MARK_AS_UNUSED(rows);
        }

        /** Sends given buffer using the attached terminal. 
         */
        void send(char const * buffer, size_t numBytes) {
            pty_->send(buffer, numBytes);
        }

        /** Sends given t++ sequence. 
         */
        void send(Sequence const & seq) {
            pty_->send(seq);
        }

    private:

        virtual void processInput(char const * bufferEnd);

        PTYSlave * pty_;
        std::thread reader_;
        char * buffer_;
        size_t bufferSize_;
        size_t bufferUnprocessed_;

    }; // tpp::TerminalClient


    /** Synchronous terminal client. 
     
        A simplified single threaded client that allows asynchronous operation. 
     */
    class TerminalClient::Sync : public TerminalClient {
    public:
        /** Returns the number of non-t++ bytes that can be read without blocking. 
         */
        size_t available() const;

        /** Blocking read. 
         */
        size_t read(char * buffer, size_t bufferSize);

        /** Returns the terminal capabilities. 
         */
        Sequence::Capabilities getCapabilities();



    protected:

        std::mutex mBuffer_;

    private:
    
        void processInput(char const * bufferEnd) {
            std::lock_guard<std::mutex> g{mBuffer_};
            TerminalClient::processInput(bufferEnd);
        }

    }; // tpp::TerminalClient::Sync


#ifdef HAHA

    /** t++ client for terminal. 
     
        Supports reading and writing both tpp sequences and normal input/output to the terminal.
     */
    class TerminalClient {
    public:
        static constexpr size_t DEFAULT_BUFFER_SIZE = 1024;

        TerminalClient(PTYSlave * pty):
            pty_{pty} {
        }

    protected:

        /** Called when normal input is received from the terminal. 
         
            The implementation should process the received input and return the number of bytes processed. These will be removed from the buffer, while any unprocessed data will be prepended to data received next. 
         */
        virtual size_t received(char const * buffer, char const * bufferEnd) = 0;

        /** Called when a t++ sequence has been received. 
         */
        virtual void receivedSequence(char const * buffer, char const * bufferEnd) = 0;

        virtual void inputEof(char const * buffer, char const * bufferEnd) {
            MARK_AS_UNUSED(buffer);
            MARK_AS_UNUSED(bufferEnd);
        }

        virtual void resized(int cols, int rows) {
            MARK_AS_UNUSED(cols);
            MARK_AS_UNUSED(rows);
        }

        /** Starts the terminal client. 
         */
        virtual void start();


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

        PTYSlave * pty_;
        std::thread reader_;

    }; // tpp::TerminalClient


    /** Simple t++ client that has provisions for sequential sending and receiving of t++ messages from a single thread. 
        
     */
    class SyncTerminalClient : public TerminalClient {
    public:

        /** Returns the number of available bytes of non t++ input that can be read. 
         */
        size_t available() const {

        }

        /** Reads the available buffer size. 
        size_t read(char * buffer, size_t bufferSize);


        /** Returns the capabilities of the terminal. 
         */
        Sequence::Capabilities getCapabilities();

    protected:

        size_t received(char const * buffer, char const * bufferEnd) override {
            ReceiveEvent::Payload p{std::make_pair(buffer, bufferEnd)};
            onInput(p, this);
        }

        void receivedSequence(char const * buffer, char const * bufferEnd) override;





        Sequence::Kind waitMessageKind_;

    }; // tpp::SequentialTerminalClient

#endif

} // namespace tpp