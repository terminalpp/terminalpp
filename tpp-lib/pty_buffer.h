#pragma once

#include <thread>

#include "pty.h"

namespace tpp {

    /** Wraps around given PTY and provides a buffered input. 

        Determine what destructor does. And so on, move the buffer from terminal here. Then revisit the other classes if the PTY buffer can be reused (such as terminal client, etc)

     */
    template<typename T>
    class PTYBuffer {
    public:

        static constexpr size_t DEFAULT_BUFFER_SIZE = 1024;
        static constexpr size_t MAX_BUFFER_SIZE = 1024 * 1024;

        virtual ~PTYBuffer() {
            if (pty_ != nullptr)
                terminatePty();
        }

        T * pty() {
            return pty_;
        }

    protected:

        explicit PTYBuffer(T * pty):
            pty_{pty} {
        }


        virtual size_t received(char * buffer, char const * end) = 0;

        virtual void ptyTerminated(ExitCode exitCode) {
            MARK_AS_UNUSED(exitCode);
        }

        void startPTYReader() {
            reader_ = std::thread{[this](){
                size_t unprocessed = 0;
                size_t bufferSize = DEFAULT_BUFFER_SIZE;
                char * buffer = new char[DEFAULT_BUFFER_SIZE];
                while (true) {
                    size_t available = pty_->receive(buffer + unprocessed, bufferSize - unprocessed);
                    // if no more bytes were read, then the PTY has been terminated, exit the loop
                    if (available == 0 && pty_->terminated())
                        break;
                    available += unprocessed;
                    unprocessed = available - received(buffer, buffer + available);
                    // copy the unprocessed bytes at the beginning of the buffer
                    memcpy(buffer, buffer + available - unprocessed, unprocessed);
                    // grow the buffer if unprocessed == bufferSize
                    if (unprocessed == bufferSize) {
                        if (bufferSize < MAX_BUFFER_SIZE) {
                            bufferSize *= 2;
                            char * b = new char[bufferSize];
                            memcpy(b, buffer, unprocessed);
                            delete [] buffer;
                            buffer = b;
                        } else {
                            unprocessed = 0;
                            LOG() << "Buffer overflow, discarding " << bufferSize << " bytes";
                        }
                    }
                }
                ptyTerminated(pty_->exitCode());
            }};
        }

        void terminatePty() {
            ASSERT(pty_ != nullptr);
            pty_->terminate();
            reader_.join();
            delete pty_;
            pty_ = nullptr;
        }

        void send(char const * what, size_t size) {
            pty_->send(what, size);
        }

        T * pty_;


    private:


        std::thread reader_;

    }; // tpp::PTYBuffer

} // namespace tpp