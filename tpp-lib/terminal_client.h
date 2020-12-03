#pragma once

#include <thread>
#include <algorithm>
#include <condition_variable>

#include "helpers/helpers.h"
#include "helpers/process.h"
#include "helpers/char.h"
#include "pty.h"
#include "sequence.h"

namespace tpp {

    class NackError : public Exception {
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

        using ResizeEvent = PTYSlave::ResizeEvent;

    protected:

        explicit TerminalClient(PTYSlave * pty);

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

        virtual void resized(ResizeEvent::Payload & e) {
            MARK_AS_UNUSED(e);
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

        virtual void processInput(char * start, char const * end);

    private:

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

        explicit Sync(PTYSlave * pty):
            TerminalClient{pty},
            timeout_{1000},
            attempts_{10},
            result_{nullptr},
            processed_{0} {
        }

        std::pair<int, int> size() const {
            return pty_->size();
        }

        /** Returns the number of non-t++ bytes that can be read without blocking. 
         */
        size_t available() const {
            std::lock_guard<std::mutex> g{mBuffer_};
            return bufferUnprocessed_ - processed_;
        }

        /** Blocking read. 
         */
        size_t read(char * buffer, size_t bufferSize) {
            std::unique_lock<std::mutex> g{mBuffer_};
            while (bufferUnprocessed_ - processed_ == 0)
                dataReady_.wait(g);
            size_t result = std::min(bufferSize, bufferUnprocessed_ - processed_);
            memcpy(buffer, buffer_ + processed_, result);
            bufferUnprocessed_ += result;
            return result;
        }

        using TerminalClient::send;

        /** Returns the terminal capabilities. 
         */
        //@{
        Sequence::Capabilities getCapabilities(size_t timeout, size_t attempts);

        Sequence::Capabilities getCapabilities(size_t timeout) {
            return getCapabilities(timeout, 1);
        }

        Sequence::Capabilities getCapabilities() {
            return getCapabilities(timeout_, 1);
        }
        //@}


        //@{
        size_t openFileTransfer(std::string const & host, std::string const & filename, size_t size, size_t timeout, size_t attempts);

        size_t openFileTransfer(std::string const & host, std::string const & filename, size_t size, size_t timeout) {
            return openFileTransfer(host, filename, size, timeout, attempts_);
        }

        size_t openFileTransfer(std::string const & host, std::string const & filename, size_t size) {
            return openFileTransfer(host, filename, size, timeout_, attempts_);
        }
        //@}

        //@{
        Sequence::TransferStatus getTransferStatus(size_t id, size_t timeout, size_t attempts);

        Sequence::TransferStatus getTransferStatus(size_t id, size_t timeout) {
            return getTransferStatus(id, timeout, attempts_);
        }

        Sequence::TransferStatus getTransferStatus(size_t id) {
            return getTransferStatus(id, timeout_, attempts_);
        }
        //@}

        //@{
        void viewRemoteFile(size_t id, size_t timeout, size_t attempts);

        void viewRemoteFile(size_t id, size_t timeout) {
            viewRemoteFile(id, timeout, attempts_);
        }

        void viewRemoteFile(size_t id) {
            viewRemoteFile(id, timeout_, attempts_);
        }
        //@}



    protected:

        size_t received(char const * buffer, char const * bufferEnd) override {
            for (char const * i = buffer; i != bufferEnd; ++i)
                if (*i == '\003') // Ctrl + C
#if (defined ARCH_UNIX)
                    raise(SIGINT);
#else
                    exit(EXIT_FAILURE);
#endif
            dataReady_.notify_all();
            return 0;
        }

        void receivedSequence(Sequence::Kind kind, char const * payload, char const * payloadEnd) override;

        void processInput(char * start, char const * end) override {
            std::lock_guard<std::mutex> g{mBuffer_};
            TerminalClient::processInput(start + processed_, end);
            processed_ = 0;
        }

        /** Transmits the sequence and waits for the response to arrive within the client's timeout. 
         */
        
        void transmit(Sequence const & send, Sequence & receive, size_t timeout, size_t attempts);

        bool responseCheck(Sequence::Kind kind, char const * payload, char const * payloadEnd);

        /** timeout for t++ sequence responses in milliseconds. 
         */
        size_t timeout_;

        /** Number of attempts a request with a corresponding response will be attempted before error. 
         */
        size_t attempts_;

        mutable std::mutex mBuffer_;
        mutable std::condition_variable dataReady_;

        std::mutex mSequences_;
        std::condition_variable sequenceReady_;
        Sequence * volatile result_;
        Sequence const * volatile request_;
        /** Number of bytes processed by the read() method. */
        size_t processed_;

    }; // tpp::TerminalClient::Sync

} // namespace tpp