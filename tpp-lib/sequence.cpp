#if (defined ARCH_LINUX)
#include <unistd.h>
#endif

#include <iostream>

#include <thread>

#include "helpers/char.h"
#include "helpers/string.h"

#include "terminal.h"

#include "sequence.h"

namespace tpp {

    Sequence Sequence::Parse(char * & start, char const * end) {
        Sequence result{};
        char * x = start;
        if (x == end) {
            result.id_ = Incomplete;
            return result;
        }
        // read the id
        if (helpers::IsDecimalDigit(*x)) {
            int id = 0;
            do {
                id = id * 10 + helpers::DecCharToNumber(*x++);
            } while (x != end && helpers::IsDecimalDigit(*x));
            // skip the ';' after the id if present
            if (x != end && *x == ';')
                ++x;
            result.id_ = id;
        }
        // read the contents
        char * valueStart = x;
        while (x != end) {
            if (*x == helpers::Char::BEL) {
                result.payload_ = std::string{valueStart, static_cast<size_t>(x - valueStart)};
                start = ++x;
                return result;
            }
            ++x;
        }
        result.id_ = Incomplete;
        return result;
    }

/*
#if (defined ARCH_UNIX)
    Sequence Sequence::Read(int fileno) {
        Sequence result{};
        // first parse the id
        {
            int id = 0;
            while (true) {
                char c;
                if (::read(fileno, &c, 1) == 0) 
                    return result; // EOF - invalid
                if (c == ';') {
                    result.id_ = id;
                    break;
                } else if (c == helpers::Char::BEL) {
                    result.id_ = id;
                    return result;
                } else if (helpers::IsDecimalDigit(c)) {
                    id = id * 10 + helpers::DecCharToNumber(c);
                } else {
                    break; // keep invalid, but parse the payload 
                }
            }
        }
        // now parse the payload
        while (true) {
            char c;
            if (::read(fileno, &c, 1) == 0) {
                result.id_ = Invalid;
                return result; 
            }
            if (c == helpers::Char::BEL)
                return result; // valid tpp sequence
            // otherwise append what we have read to the payload
            result.payload_ += c;
        }
    }

    Sequence Sequence::WaitAndRead(int fileno, size_t timeout) {
        {
            NonBlockingInput nb{fileno};
            size_t state = 0; // 0 = nothing, 1 = ESC parsed, 2 = ] parsed, 3 = + parsed 
            while (state != 3) {
                char c;
                switch (::read(fileno, &c, 1)) {
                    case -1: // nothing to read
                        std::this_thread::sleep_for(std::chrono::milliseconds(1));
                        if (--timeout == 0) {
                            std::cout << "timeout" << std::endl;    
                            return Sequence{}; // timeout
                        }
                        break;
                    case 0:
                        return Sequence{}; // EOF 
                    case 1:
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
                                    state = 3;
                                } else {
                                    state = 0;
                                    ASSERT(false) << "Non TPP sequence on input";
                                }
                                break;
                            default:
                                state = 0;
                                ASSERT(false) << "Non TPP sequence on input";
                        }
                        break;
                    default:
                        UNREACHABLE;
                }
            }
            ASSERT(state == 3);
        }
        return Read(fileno);
    }

#endif
*/
    namespace request {

        NewFile::NewFile(Sequence && from):
            Sequence(std::move(from)),
            size_{0} {
            if (id() == Sequence::NewFile) {
                std::vector<std::string> parts = helpers::Split(payload(), ";");
                if (parts.size() >= 4) {
                    try {
                        size_ = std::stoul(parts[0]);
                        hostname_ = parts[1];
                        filename_ = parts[2];
                        remotePath_ = parts[3];
                    } catch (...) {
                        // do nothing
                    }
                }
            }
        }

        Send::Send(Sequence && from):
            Sequence(std::move(from)),
            fileId_{-1},
            data_{nullptr},
            size_{0} {
            if (id() == Sequence::Send) {
                size_t dataStart = payload().find(';');
                try {
                    fileId_ = std::stoi(payload().substr(0, dataStart));
                    data_ = payload().c_str() + dataStart + 1;
                    size_ = payload().size() - dataStart - 1;
                    // and now decode the payload data (we know the decoded size is <= so we can decode in place)
                    char const * r = data_;
                    size_t w = dataStart + 1;
                    char const * e = data_ + size_;
                    while (r < e) 
                        payload_[w++] = Terminal::Decode(r);
                    size_ = w - dataStart - 1;
                } catch (...) {
                    // do nothing
                }
            }
        }

        OpenFile::OpenFile(Sequence && from):
            Sequence(std::move(from)),
            fileId_(-1) {
            if (id() == Sequence::OpenFile) {
                try {
                    fileId_ = std::stoi(payload());
                } catch (...) {
                    // do nothing
                }
            }
        }

    } // namespace tpp::request

    namespace response {

        Capabilities::Capabilities(Sequence && from):
            Sequence(std::move(from)),
            version_{-1} {
            if (id() == Sequence::Capabilities) {
                std::vector<std::string> parts = helpers::Split(payload(), ";");
                if (parts.size() > 0) {
                    try {
                        version_ = std::stoi(parts[0]);
                    } catch (...) {
                        // do nothing, keep version -1 (invalid)
                    }
                }
            }
        }

        NewFile::NewFile(Sequence && from):
            Sequence(std::move(from)),
            fileId_{-1} {
            if (id() == Sequence::NewFile) {
                std::vector<std::string> parts = helpers::Split(payload(), ";");
                if (parts.size() > 0) {
                    try {
                        fileId_ = std::stoi(parts[0]);
                    } catch (...) {
                        // do nothing, keep version -1 (invalid)
                    }
                }
            }
        }

    } // namespace tpp::response



} // namespace tpp