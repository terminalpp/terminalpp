#pragma once

#include "ui-terminal/pty.h"

namespace tpp {

    using ui::PTY;

    /** Terminal++ PTY. 
     
        PTY++ is itself a PTY client for a basic OS pseudoterminal connection and provides the multiplexing and additional `t++` features. 
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

        virtual size_t processInput(char const * buffer, char const * bufferEnd) override {
            size_t size = bufferEnd - buffer;
            defaultChannel_->receive(buffer, size);
            return size;
        }


    private:

        Proxy * defaultChannel_;
    };


} // namespace tpp

