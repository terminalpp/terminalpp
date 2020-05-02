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

            void terminate() override {
                pty_->terminate(this);
            }

        protected:

            void resize(int cols, int rows) override {
                pty_->resize(this, cols, rows);
            }

            void send(char const * buffer, size_t bufferSize) override {
                pty_->send(this, buffer, bufferSize);
            }

        private:
            size_t channel_;
            PtyPP * pty_;
        };

    protected:

        void terminate(ProxyPTY * sender) {

        }

        void resize(ProxyPTY * sender, int cols, int rows) {

        }

        void send(ProxyPTY * sender, char const * buffer, size_t bufferSize) {

        }


        virtual void ptyTerminated(ExitCode exitCode) override {

        }


        virtual size_t processInput(char const * buffer, char const * bufferEnd) override {

        }


    private:
        /** Channels to which the pseudoterminal demultiplexes the input. 
         */
        std::unordered_map<size_t, ProxyPTY *> channels_;
    };


} // namespace tpp

