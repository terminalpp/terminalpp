#pragma once

#include "helpers/process.h"

namespace ui2 {

    using helpers::ExitCode;

    /** Pseudoterminal connection. 

        TODO how are clients attached and by whom?
     
     */
    class PTY {
    public:

        /** Client for a pseudoterminal connection. 
         
            Deals with the interaction between the PTY. Provides buffering of unprocessed input and methods called on PTY events and actions. 

            TODO maybe this should move to the terminal proper, and the client should only define the API - depending on how the PTYpp will look.
         */
        class Client {
        public:

            Client():
                pty_{nullptr},
                inputBuffer_{new char[DEFAULT_BUFFER_SIZE]},
                inputBufferUnprocessed_{0},
                inputBufferSize_{DEFAULT_BUFFER_SIZE} {
            }

            virtual ~Client() noexcept(false) {
                delete [] inputBuffer_;
            }
            
            /** Returns the PTY connected to the client. 
             */
            PTY * pty() const {
                return pty_;
            }

        protected:

            friend class PTY;

            /** Resizes the attached pseudoterminal. 
             */
            void ptyResize(int cols, int rows) {
                if (pty_ != nullptr)
                    pty_->resize(cols, rows);
            }

            /** Sends the data to the attached pseudoterminal. 
             */
            void ptySend(char const * buffer, size_t bufferSize) {
                if (pty_ != nullptr)
                    pty_->send(buffer, bufferSize);
            }

            /** Called when new data has been received via the pseudoterminal. 
             */
            void ptyReceive(char const * buffer, size_t bufferSize) {
                // first a fastpath if there is no unprocessed bytes left, we can avoid the copying
                if (inputBufferUnprocessed_ == 0) {
                    size_t processed = processInput(buffer, buffer + bufferSize);
                    if (processed != bufferSize) {
                        inputBufferUnprocessed_ = bufferSize - processed;
                        if (inputBufferUnprocessed_ > inputBufferSize_)
                            growInputBuffer(inputBufferUnprocessed_ * 2);
                        memcpy(inputBuffer_, buffer + processed, inputBufferUnprocessed_);
                    }
                // copy the inpit buffer after the unprocessed characters
                } else {
                    size_t available = inputBufferUnprocessed_ + bufferSize;
                    if (available > inputBufferSize_)
                        growInputBuffer(available * 2);
                    memcpy(inputBuffer_ + inputBufferUnprocessed_, buffer, bufferSize);
                    size_t processed = processInput(inputBuffer_, inputBuffer_ + available);
                    inputBufferUnprocessed_ = available - processed;
                    if (inputBufferUnprocessed_ > 0) 
                        memcpy(inputBuffer_, inputBuffer_ + processed, inputBufferUnprocessed_);
                }
            }

            /** Called when the attached process has been terminated. 
             */
            virtual void ptyTerminated(ExitCode exitCode) {
                // do nothing, children can reimplement to react. 
            }

            virtual size_t processInput(char const * buffer, char const * bufferEnd) = 0;

        private:

            /** Grows the input buffer to given size. 
             */
            void growInputBuffer(size_t newSize) {
                ASSERT(newSize > inputBufferSize_);
                char * oldBuffer = inputBuffer_;
                inputBufferSize_ = newSize;
                inputBuffer_ = new char [inputBufferSize_];
                memcpy(inputBuffer_, oldBuffer, inputBufferUnprocessed_);
                delete [] oldBuffer;
            }

            PTY * pty_;

            char * inputBuffer_;
            size_t inputBufferUnprocessed_;
            size_t inputBufferSize_;

        }; 

        PTY(Client * client):
            client_{client} {
        }

        /** Returns the client the PTY is attached to. 
         */
        Client * client() const {
            return client_;
        }

        /** Terminates the attached process.
         
            Upon calling, the attached terminal process should be terminated and the client will be notified. There is no guarantee that the PTY is terminated when the function returns. 
         */
        virtual void terminate() = 0;

        static constexpr size_t DEFAULT_BUFFER_SIZE = 1024;

    protected:

        /** Resizes the pseudoterminal. 
         
            Must be provided by the implementation. 
         */
        virtual void resize(int cols, int rows) = 0;

        /** Sends given buffer to the PTY. 
         
            Must be provided by the PTY implementation.
         */
        virtual void send(char const * buffer, size_t bufferSize) = 0;

        /** Called by the implementation when new data is received. 
         
            If client exists, makes the client process the data, otherwise does nothing. 
         */
        void receive(char const * buffer, size_t bufferSize) {
            if (client_ != nullptr)
                client_->ptyReceive(buffer, bufferSize);
        }

        // TODO archive the exit code here and do stuff? 
        void terminated(ExitCode exitCode) {
            if (client_ != nullptr)
                client_->ptyTerminated(exitCode);
        }

        virtual ~PTY() {
        }

        Client * client_;

    }; // ui::AnsiTerminal::PTY



} // namespace ui