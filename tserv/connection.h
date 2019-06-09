#pragma once

#include <thread>

#include "vterm/local_pty.h"

namespace tserv {

    /** Represents the target process connection. 
     */
    class Connection {
    public:
        Connection(helpers::Command const & cmd, size_t bufferSize = 10240):
            pty_(cmd),
            buffer_(new char[bufferSize]) {
        }

        void start() {

        }

        void terminate() {

        }

        

    private:
        vterm::LocalPTY pty_;

        char * buffer_;




    }; // tserv::Connection



} // namespace tserv