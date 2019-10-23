#pragma once

#if (defined ARCH_LINUX)
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
     */
    class Terminal {
    public:

        static constexpr size_t NoInputAvailable = std::numeric_limits<size_t>::max();
        static constexpr size_t InputEoF = 0;

        Terminal():
            timeout_(500) {
        }

        virtual ~Terminal() {
        }

        size_t timeout() const {
            return timeout_;
        }

        void setTimeout(size_t value) {
            timeout_ = value;
        }

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
        
        Sequence readSequence();

        response::Capabilities getCapabilities();

        int newFile(std::string const & path, size_t size);

        void transmit(int fileId, size_t offset, char const * data, size_t numBytes);

        /** Returns the number of bytes correctly received for the given file. 
         */
        size_t transferStatus(int fileId);

        void openFile(int fileId);

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

        void waitForSequence();


    private:

        void encodeBuffer(char const * data, size_t numBytes);

        std::vector<char> buffer_;
        size_t timeout_;

    }; 

#if (defined ARCH_LINUX)

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
