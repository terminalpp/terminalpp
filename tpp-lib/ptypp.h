#pragma once

#include "ui-terminal/pty.h"

namespace tpp {

    using ui::PTY;

    /** Terminal++ PTY. 
     
        PTY++ is itself a PTY client for a basic OS pseudoterminal connection and provides the multiplexing and additional `t++` features. 

        - multiple channels can be multiplexed into single PTY
        - each channel can transfer files
     */
    class PtyPP : public PTY::Client {
    public:
        /** Proxy PTY. 
         
            Demultiplexed PTY. 
         */
        class Proxy : public PTY {
        public:

            Proxy(PTY::Client * client, PtyPP * ptypp, size_t channel):
                PTY{client},
                ptypp_{ptypp},
                channel_{channel} {
            }

            size_t channel() const {
                return channel_;
            }

            void terminate() override {
                ptypp_->terminate(this);
            }

        protected:
            friend class PtyPP;

            void resize(int cols, int rows) override {
                ptypp_->resize(this, cols, rows);
            }

            void send(char const * buffer, size_t bufferSize) override {
                ptypp_->send(this, buffer, bufferSize);
            }

        private:
            PtyPP * ptypp_;
            size_t channel_;
        }; // tpp::PtyPP::Proxy

        PtyPP(PTY::Client * defaultClient):
            defaultChannel_{new Proxy{defaultClient, this, 0}} {
        }

        ~PtyPP() override {
            delete defaultChannel_;
        }

    protected:

        void terminate(Proxy * sender) {
            if (sender->channel() == 0) {
                pty()->terminate();
            } else {
                NOT_IMPLEMENTED;
            }
        }

        void resize(Proxy * sender, int cols, int rows) {
            if (sender->channel() == 0) {
                ptyResize(cols, rows);
            } else {
                NOT_IMPLEMENTED;
            }
        }

        void send(Proxy * sender, char const * buffer, size_t bufferSize) {
            if (sender->channel() == 0) {
                ptySend(buffer,bufferSize);
            } else {
                NOT_IMPLEMENTED;
            }
        }

        void ptyTerminated(ExitCode exitCode) override {
            defaultChannel_->terminated(exitCode);
        }

        /** Processes the input from the main PTY. 
         
         */
        virtual size_t processInput(char const * buffer, char const * bufferEnd) override {
            size_t processed = 0;
            while (buffer != bufferEnd) {
                char const * tppStart = bufferEnd;
                char const * i = buffer;
                while (i < bufferEnd - 2) {
                    if (i[2] == '+' && i[0] == '\033' && i[1] == 'P') {
                        tppStart = i;
                        break;
                    }
                    ++i;
                }
                if (tppStart == bufferEnd - 2)
                    tppStart = bufferEnd;
                // if there are characters before the TPP sequence, send them to default channel
                if (tppStart != buffer) {
                    processed += tppStart - buffer;
                    defaultChannel_->receive(buffer, tppStart - buffer);
                }
                // parse the size of the message, which is stored as hexadecimal number of bytes followed by a semicolon
                size_t msgSize = 0;
                unsigned digit = 0;
                i = tppStart + 3; 
                while (true) {
                    // if we got to the end of the buffer without parsing the proper size, more needs to be received
                    if (i >= bufferEnd)
                        return processed;
                    // we have the size
                    if (*i == ';') {
                        char const * msgStart = i + 1; // past the semicolon
                        // if the message is not entirely in the buffer, we need to read more
                        if (msgStart + msgSize >= bufferEnd)
                            return processed;
                        if (msgStart[msgSize] != Char::BEL) {
                            LOG() << "Error";
                            break;
                        }
                        // actually process the tpp message
                        processTpp(msgStart, msgStart + msgSize);
                        // update number of processed elements
                        processed += msgStart - tppStart + msgSize + 1;
                        tppStart = msgStart + msgSize + 1; // past the message and its BEL terminator
                        break;
                    }
                    // otherwise it has to be hexadecimal number determining the size of the message
                    if (!Char::IsHexadecimalDigit(*i, digit)) {
                        LOG() << "Error";
                        break;
                    }
                    ++i;
                    msgSize = (msgSize * 10) + digit;
                }
                // set the buffer start to what is in tppStart, which skips over the tpp message if successful, but addds the tpp message to the default channel if its format is not valid 
                buffer = tppStart;
            }
            return processed;
        }

        /** Procesess the contents of tpp sequence. 
         */
        void processTpp(char const * buffer, char const * bufferEnd) {
            LOG() << "TPP message received, size: " << (bufferEnd - buffer);
        }

    private:

        Proxy * defaultChannel_;
    };


} // namespace tpp

