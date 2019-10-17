#if (defined ARCH_LINUX)

#include <limits>
#include <thread>

#include "helpers/filesystem.h"
#include "helpers/process.h"

#include "terminal.h"

namespace tpp {

    namespace {

        constexpr char const * TPP_START = "\033]+";
        constexpr char const * TPP_END = "\007";

    } // anonymous namespace

    Sequence Terminal::readSequence(size_t timeout) {
        Sequence result{};
        // first wait for the tpp sequence to start, discarding any non-tpp traffic, if timeout, return invalid sequence
        if (! waitForSequence(timeout)) 
            return result;
        // now parse the id
        char c;
        int id = 0;
        while (true) {
            if (readBlocking(&c, 1) == 0)
                return result; // EOF - invalid
            if (c == ';') {
                result.id_ = id;
                break;
            }
            if (c == helpers::Char::BEL) {
                result.id_ = id;
                return result;
            }
            if (helpers::IsDecimalDigit(c))
                id = id * 10 + helpers::DecCharToNumber(c);
            else
                break; // keep invalid, parse the payload
        }
        // and the payload
        while (true) {
            if (readBlocking(&c, 1) == 0) {
                result.id_ = Sequence::Invalid;
                return result;
            }
            if (c == helpers::Char::BEL)
                return result;
            result.payload_ += c;
        }
    }

    response::Capabilities Terminal::getCapabilities() {
        {
            std::string x(STR(TPP_START << Sequence::Capabilities << TPP_END));
            sendSequence(x.c_str(), x.size());
        }
        return response::Capabilities{readSequence()};
    }

    int Terminal::newFile(std::string const & path, size_t size) {
        {
            std::string x(STR(TPP_START << Sequence::NewFile << ";" 
                << size << ";" 
                << helpers::GetHostname() << ";"
                << helpers::GetFilename(path) << ";"
                << path << TPP_END));
            sendSequence(x.c_str(), x.size());
        }
        return response::NewFile{readSequence()}.fileId();
    }

    bool Terminal::transmit(int fileId, char const * data, size_t numBytes, size_t ackTimeout) {
        {
            beginSequence();
            std::string x(STR(TPP_START << Sequence::Data << ";" << fileId << ";"));
            send(x.c_str(), x.size());
            encodeBuffer(data, numBytes);
            send(buffer_.data(), buffer_.size());
            send(TPP_END, 1);
            endSequence();
            buffer_.clear();
        }
        return readSequence(ackTimeout).id() == Sequence::Ack;
    }

    void Terminal::openFile(int fileId) {
        std::string x(STR(TPP_START << Sequence::OpenFile << ";" << fileId << TPP_END));
        sendSequence(x.c_str(), x.size());
    }

    bool Terminal::waitForSequence(size_t timeout) {
        int state = 0; // 0 = nothing, 1 = ESC parsed, 2 = ] parsed, 3 = + parsed 
        char c;
        int t = (timeout == 0) ? timeout_ : timeout;
        while (true) {
            switch (readNonBlocking(&c, 1)) {
                case NoInputAvailable:
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    if (--t == 0) 
                        return false;
                    break;
                case InputEoF:
                    return false;
                default: {
                    switch (c) {
                        case '\033':
                            state = 1;
                            break;
                        case ']':
                            if (state == 1) {
                                state = 2;
                            } else {
                                state = 0;
                                ASSERT(false) << "Non TPP sequence on input";
                            }
                            break;
                        case '+':
                            if (state == 2) {
                                return true;
                            } else {
                                state = 0;
                                ASSERT(false) << "Non TPP sequence on input";
                            }
                            break;
                        default:
                            state = 0;
                            ASSERT(false) << "Non TPP sequence on input";
                    }
                }
            }
        }
        return false;
    }

    void Terminal::encodeBuffer(char const * data, size_t numBytes) {
        buffer_.clear();
        while (numBytes-- > 0) {
            switch (*data) {
                case helpers::Char::BEL:
                case helpers::Char::ESC:
                case '`':
                    buffer_.push_back('`');
                    buffer_.push_back(helpers::ToHexDigit(*data >> 4));
                    buffer_.push_back(helpers::ToHexDigit(*data & 0xf));
                    break;
                default:
                    buffer_.push_back(*data);
            }
            ++data;
        }
    }


#if (defined ARCH_LINUX)

    // StdTerminal

    StdTerminal::StdTerminal(int in, int out):
        in_(in),
        out_(out),
        blocking_(true),
        insideTmux_(false) {
        tcgetattr(in_, & backup_);
        termios raw = backup_;
        raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
        raw.c_oflag &= ~(OPOST);
        raw.c_cflag |= (CS8);
        raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
        tcsetattr(in_, TCSAFLUSH, & raw);
        checkTmux();
    }

    StdTerminal::~StdTerminal() {
        tcsetattr(in_, TCSAFLUSH, & backup_);
    }

    void StdTerminal::beginSequence() {
        if (insideTmux_)
            ::write(out_, "\033Ptmux;", 7);
    }

    void StdTerminal::endSequence() {
        if (insideTmux_)
            ::write(out_, "\033\\", 2);
    }

    void StdTerminal::send(char const * buffer, size_t numBytes) {
        if (insideTmux_) {
            // we have to properly escape the buffer
            size_t start = 0;
            size_t end = 0;
            while (end < numBytes) {
                if (buffer[end] == '\033') {
                    if (start != end)
                        ::write(out_, buffer+start, end - start);
                    ::write(out_, "\033\033", 2);
                    start = ++end;
                } else {
                    ++end;
                }
            }
            if (start != end)
                ::write(out_, buffer+start, end - start);
        } else {
            ::write(out_, buffer, numBytes);
        }
    }

    size_t StdTerminal::readBlocking(char * buffer, size_t bufferSize) {
        setBlocking(true);
        return ::read(in_, buffer, bufferSize);
    }

    size_t StdTerminal::readNonBlocking(char * buffer, size_t bufferSize) {
        ASSERT(bufferSize < NoInputAvailable);
        setBlocking(false);
        int x = ::read(in_, buffer, bufferSize);
        return (x == -1) ? NoInputAvailable : x;
    }

    void StdTerminal::checkTmux() {
        if (helpers::Environment::Get("TMUX") != nullptr)
            insideTmux_ = true;
    }

#endif

} // namespace tpp
#endif