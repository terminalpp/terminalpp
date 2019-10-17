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

        Data::Data(Sequence && from):
            Sequence(std::move(from)),
            fileId_{-1},
            data_{nullptr},
            size_{0} {
            if (id() == Sequence::Data) {
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

        Ack::Ack(Sequence && from):
            Sequence(std::move(from)) {
            // do nothing - if seq id is ack, the ack is valid
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