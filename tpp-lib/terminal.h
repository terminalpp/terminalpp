#pragma once

#if (defined ARCH_UNIX)
#include <termios.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#endif

#include <vector>
#include <limits>

#include "helpers/helpers.h"
#include "helpers/char.h"

#include "sequence.h"

namespace tpp {

    /** The terminal is an abstraction over the PTY. 

        It defines the basic API necessary to implement the reading and writing of `t++` extra escape sequences and then provides methods for transmitting each command and its associated response (if any). 

        Actual terminal implementation should inherit from the tpp::Terminal class and implement the holes for the basic I/O functionality.  

        TODO the terminal should ideally support also `<<` and so on so that it can be used instead of std::cout.
     */
    class Terminal {
    public:

        static constexpr size_t NoInputAvailable = std::numeric_limits<size_t>::max();
        static constexpr size_t InputEoF = 0;

        Terminal():
            timeout_(1000) {
        }

        virtual ~Terminal() {
        }

        /** Determines the timeout in milliseconds the server has to respond to any of the messages. 
         */
        size_t timeout() const {
            return timeout_;
        }

        void setTimeout(size_t value) {
            timeout_ = value;
        }

        /** Reads a `t++` sequence from the terminal, ignoring any non-tpp input. 
         
            Throws the helpers::Timeout error if no `t++` sequence appears before the timeout. 
         */
        Sequence readSequence();

        /** Queries the capabilities of the terminal. 
         
            Throws the helpers::Timeout error on timeout, which means no `terminal++` is attached. 
         */
        Sequence::CapabilitiesResponse getCapabilities();

        /** Requests the connection id for new local file to be transmitted to the server. 
         
            Requires the local path and size of the file. 
         */
        Sequence::NewFileResponse newFile(std::string const & path, size_t size);

        /** Transmits given block of data. 
         */
        void transmit(int fileId, size_t offset, char const * data, size_t numBytes);

        /** Returns the transfer status of the specified connection id. 
         */
        Sequence::TransferStatusResponse transferStatus(int fileId);

        /** Instructs the attached `terminal++` to open the file associated with given connection id. 
         */
        void openFile(int fileId);

        /** Decodes single character from `t++` encoded data, advancing the buffer read accordingly. 
         */
        static char Decode(char const * & buffer, char const * end) {
            ASSERT(buffer != end);
            if (*buffer != '`') {
                return *(buffer++);
            } else {
                if (buffer + 3 > end)
                    THROW(helpers::IOError()) << "Not enough data to decode quoted value";
                char result = static_cast<char>(helpers::ParseHexNumber(buffer + 1, 2));
                buffer += 3;
                return result;                
            }
        }

    protected:

        virtual void beginSequence() = 0;
        virtual void endSequence() = 0;
        void sendSequence(char const * buffer, size_t numBytes) {
            beginSequence();
            send(buffer, numBytes);
            endSequence();
        }
        virtual void send(char const * buffer, size_t numBytes) = 0;
        virtual size_t readBlocking(char * buffer, size_t bufferSize) = 0;
        virtual size_t readNonBlocking(char * buffer, size_t bufferSize) = 0;

        void waitForSequence();

    private:

        void encodeBuffer(char const * data, size_t numBytes);

        std::vector<char> buffer_;
        size_t timeout_;

    }; 

#if (defined ARCH_UNIX)

    /** Provides terminal wrapper over standard input and output streams. 
     */
    class StdTerminal : public Terminal {
    public:
        StdTerminal(int in = STDIN_FILENO, int out = STDOUT_FILENO);

        ~StdTerminal() override; 

        void beginSequence() override;
        void endSequence() override;
        void send(char const * buffer, size_t numBytes) override;
        size_t readBlocking(char * buffer, size_t bufferSize) override;
        size_t readNonBlocking(char * buffer, size_t bufferSize) override;

    protected:

        void setBlocking(bool value = true) {
            if (blocking_ == value)
                return;
            blocking_ = value;
            if (blocking_)
                fcntl(in_, F_SETFL, fcntl(STDIN_FILENO, F_GETFL) & ~O_NONBLOCK);
            else
                fcntl(in_, F_SETFL, fcntl(STDIN_FILENO, F_GETFL) | O_NONBLOCK);
        }

        void checkTmux();
        
        int in_;
        int out_;
        bool blocking_;
        termios backup_;
        bool insideTmux_;
    };

#endif

} // namespace tpp
