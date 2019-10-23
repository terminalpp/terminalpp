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
            // ESC character cannot appear in the payload, if it does, we are leaking real escape codes and should stop.
            if (*x == helpers::Char::ESC) {
                result.id_ = Invalid;
                start = x;
                return result;
            }
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
                // determine the delimiters for the fileId and transmitted size
                // TODO perhaps this should be a better parser in the end
                size_t dataSizeStart = payload().find(';') + 1;
                if (dataSizeStart == std::string::npos)
                    return; // invalid
                size_t dataOffsetStart = payload().find(';', dataSizeStart) + 1; 
                if (dataOffsetStart == std::string::npos)
                    return; // invalid
                size_t dataStart = payload().find(';', dataOffsetStart) + 1;
                if (dataStart == std::string::npos)
                    return; // invalid
                try {
                    fileId_ = std::stoi(payload().substr(0, dataStart));
                    size_ = std::stoul(payload().substr(dataSizeStart, dataOffsetStart - dataSizeStart));
                    offset_ = std::stoul(payload().substr(dataOffsetStart, dataStart - dataOffsetStart));
                    // decode the contents
                    data_ = payload().c_str() + dataStart;
                    size_t w = dataStart;
                    char const * r = data_;
                    char const * e = payload().c_str() + payload().size();
                    while (r < e) 
                        payload_[w++] = Terminal::Decode(r, e);
                    // if different length of data was received than advertised, the packet is invalid
                    if (w - dataStart  != size_)
                        fileId_ = -1;
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

        TransferStatus::TransferStatus(Sequence && from):
            Sequence(std::move(from)),
            fileId_(-1) {
            if (id() == Sequence::TransferStatus) {
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

        TransferStatus::TransferStatus(Sequence && from):
            Sequence{std::move(from)},
            fileId_{-1},
            transmittedBytes_{0} {
            if (id() == Sequence::TransferStatus) {
                std::vector<std::string> parts = helpers::Split(payload(), ";");
                if (parts.size() >= 2) {
                    try {
                        fileId_ = std::stoi(parts[0]);
                        transmittedBytes_ = std::stoul(parts[1]);
                    } catch (...) {
                        // do nothing
                    }
                }
            }
        }

    } // namespace tpp::response



} // namespace tpp