#include "helpers/char.h"

#include "sequence.h"
#include "terminal_client.h"


namespace tpp {


    std::ostream & operator << (std::ostream & s, Sequence::Kind kind) {
        switch (kind) {
            case Sequence::Kind::Ack:
                s << "Sequence::Ack";
                break;
            case Sequence::Kind::GetCapabilities:
                s << "Sequence::GetCapabilities";
                break;
            case Sequence::Kind::Capabilities:
                s << "Sequence::Capabilities";
                break;
            case Sequence::Kind::Invalid:
                s << "Sequence::Invalid";
                break;
            default:
                s << "Unknown sequence " << static_cast<unsigned>(kind);
                break;
        }
        return s;
    }

    // Sequence

    char const * Sequence::FindSequenceStart(char const * buffer, char const * bufferEnd) {
        while (buffer != bufferEnd) {
            if (*buffer == '\033') {
                if (buffer + 1 == bufferEnd)
                    break;
                if (buffer[1] == 'P') {
                    if (buffer + 2 == bufferEnd || buffer[2] == '+')
                        break;
                    ++buffer;
                } 
            }
            ++buffer;
        }
        return buffer;
    }

    char const * Sequence::FindSequenceEnd(char const * buffer, char const * bufferEnd) {
        while (buffer < bufferEnd && *buffer != Char::BEL)
            ++buffer;
        return buffer;
    }

    Sequence::Kind Sequence::ParseKind(char const * & buffer, char const * bufferEnd) {
        unsigned result = 0;
        while (buffer < bufferEnd) {
            if (*buffer == ';') {
                ++buffer;
                return static_cast<Kind>(result);
            } else if (*buffer == Char::BEL) {
                return static_cast<Kind>(result);
            }
            unsigned digit = 0;
            if (! Char::IsDecimalDigit(*buffer, digit))
                return Kind::Invalid;
            result = (result * 10) + digit;
            ++buffer;
        }
        return Kind::Invalid;
    }

    void Sequence::writeTo(std::ostream & s) const {
        s << static_cast<unsigned>(kind_);
    }

    size_t Sequence::ReadUnsigned(char const * & start, char const * end) {
        size_t result = 0;
        unsigned digit = 0;
        while (start < end) {
            if (Char::IsDecimalDigit(*start, digit)) {
                result = result * 10 + digit;
                ++start;
            } else {
                if (*start == ';') {
                    ++start;
                    break;
                }
                THROW(SequenceError()) << "Expected decimal digit, but " << *start << " found in sequence payload";
            }
        }
        return result;
    }

    std::string Sequence::ReadString(char const * & start, char const * end) {
        std::string result;
        while (start != end) {
            if (*start == ';') {
                ++start;
                break;
            } else {
                result += DecodeChar(start, end);
            }
        }
        return result;
    }

    void Sequence::WriteString(std::ostream & s, std::string const & str) {
        char const * r = str.c_str();
        char const * end = r + str.size();
        while (r != end) {
            switch (*r) {
                case Char::NUL:
                case Char::BEL:
                case Char::ESC:
                case ';':
                case '`':
                    s << '`';
                    s << Char::ToHexadecimalDigit(static_cast<unsigned char>(*r) >> 4);
                    s << Char::ToHexadecimalDigit(static_cast<unsigned char>(*r) & 0xf);
                    ++r;
                    break;
                default:
                    s << (*r++);
                    break;
            }
        }
    }

    void Sequence::Encode(std::ostream & s, char const * buffer, char const * end) {
        char const * r = buffer;
        while (r != end) {
            switch (*r) {
                case Char::NUL:
                case Char::BEL:
                case Char::ESC:
                case '`':
                    s << '`';
                    s << Char::ToHexadecimalDigit(static_cast<unsigned char>(*r) >> 4);
                    s << Char::ToHexadecimalDigit(static_cast<unsigned char>(*r) & 0xf);
                    ++r;
                    break;
                default:
                    s << (*r++);
                    break;
            }
        }
    }

    void Sequence::Decode(Buffer & into, char const * buffer, char const * end) {
        char const * r = buffer;
        while (r < end) {
            into << DecodeChar(r, end);
        }
    }
    
    // Sequence::Ack

    void Sequence::Ack::writeTo(std::ostream & s) const {
        Sequence::writeTo(s);
        s << ';' << id_ << ';';
        WriteString(s, payload_);
    }

    // Sequence::Capabilities

    void Sequence::Capabilities::writeTo(std::ostream & s) const {
        Sequence::writeTo(s);
        s << ';' << version_;
    }

    // Sequence::Data

    void Sequence::Data::writeTo(std::ostream & s) const {
        Sequence::writeTo(s);
        s << ';' << id_ << ';' << packet_ << ';' << size_ << ';';
        Encode(s, payload_, payload_ + size_);
    }

    // Sequence::OpenFileTransfer

    void Sequence::OpenFileTransfer::writeTo(std::ostream & s) const {
        Sequence::writeTo(s);
        s << ';';
        WriteString(s, remoteHost_);
        s << ';';
        WriteString(s, remotePath_);
        s << ';' << size_;
    }

    // Sequence::GetTransferStatus

    void Sequence::GetTransferStatus::writeTo(std::ostream & s) const {
        Sequence::writeTo(s);
        s << ';' << id_;
    }

    // Sequence::TransferStatus;

    void Sequence::TransferStatus::writeTo(std::ostream & s) const {
        Sequence::writeTo(s);
        s << ';' << id_ << ';' << size_ << ';' << received_;
    }

    // Sequence::ViewRemoteFile

    void Sequence::ViewRemoteFile::writeTo(std::ostream & s) const {
        Sequence::writeTo(s);
        s << ';' << id_;
    }



} // namespace tpp





#ifdef HAHA

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
            result.id_ = Kind::Incomplete;
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
            result.id_ = static_cast<Kind>(id);
        }
        // read the contents
        char * valueStart = x;
        while (x != end) {
            // ESC character cannot appear in the payload, if it does, we are leaking real escape codes and should stop.
            if (*x == helpers::Char::ESC) {
                result.id_ = Kind::Invalid;
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
        result.id_ = Kind::Incomplete;
        return result;
    }

    Sequence::AckResponse::AckResponse(Sequence && seq) {
        if (seq.id_ != Kind::Ack || !seq.payload_.empty())
            THROW(SequenceError()) << "Invalid ack response " << seq;
    }

    Sequence::CapabilitiesRequest::CapabilitiesRequest(Sequence && seq) {
        if (seq.id_ != Kind::Capabilities || !seq.payload_.empty())
            THROW(SequenceError()) << "Invalid capabilities request sequence " << seq;
    }

    Sequence::CapabilitiesResponse::CapabilitiesResponse(Sequence && seq) {
        if (seq.id_ != Kind::Capabilities)
            THROW(SequenceError()) << "Invalid capabilities response sequence " << seq;
        ContentsParser cp(std::move(seq));
        version = cp.parseInt();
        cp.checkEoF();
    }

    Sequence::NewFileRequest::NewFileRequest(Sequence && seq) {
        if (seq.id_ != Kind::NewFile)
            THROW(SequenceError()) << "Invalid new file request sequence " << seq;
        ContentsParser cp(std::move(seq));
        size = cp.parseUnsigned();
        hostname = cp.parseString();
        filename = cp.parseString();
        remotePath = cp.parseString();
        cp.checkEoF();
    }

    Sequence::NewFileResponse::NewFileResponse(Sequence && seq) {
        if (seq.id_ != Kind::NewFile)
            THROW(SequenceError()) << "Invalid new file response sequence " << seq;
        ContentsParser cp(std::move(seq));
        fileId = cp.parseInt();
        cp.checkEoF();
    }

    Sequence::DataRequest::DataRequest(Sequence && seq) {
        if (seq.id_ != Kind::Data)
            THROW(SequenceError()) << "Invalid data request sequence " << seq;
        ContentsParser cp(std::move(seq));
        fileId = cp.parseInt();
        size_t size = cp.parseUnsigned();
        offset = cp.parseUnsigned();
        data = cp.parseEncodedData();
        if (size != data.size())
            THROW(SequenceError()) << "Data packet size mismatch";
        cp.checkEoF();
    }

    Sequence::TransferStatusRequest::TransferStatusRequest(Sequence && seq) {
        if (seq.id_ != Kind::TransferStatus)
            THROW(SequenceError()) << "Invalid transfer status request sequence " << seq;
        ContentsParser cp(std::move(seq));
        fileId = cp.parseInt();
        cp.checkEoF();
    }

    Sequence::TransferStatusResponse::TransferStatusResponse(Sequence && seq) {
        if (seq.id_ != Kind::TransferStatus)
            THROW(SequenceError()) << "Invalid transfer status response sequence " << seq;
        ContentsParser cp(std::move(seq));
        fileId = cp.parseInt();
        transmittedBytes = cp.parseUnsigned();
        cp.checkEoF();
    }

    Sequence::OpenFileRequest::OpenFileRequest(Sequence && seq) {
        if (seq.id_ != Kind::OpenFile)
            THROW(SequenceError()) << "Invalid open file request sequence " << seq;
        ContentsParser cp(std::move(seq));
        fileId = cp.parseInt();
        cp.checkEoF();
    }

    int Sequence::ContentsParser::parseInt() {
        if (i_ >= payload_.size())
            THROW(SequenceError()) << "Integer expected, but EoF found in sequence " << payload_;
        int result = 0;
        int multiplier = 1;
        if (payload_[i_] == '-') {
            multiplier = -1;
            ++i_;
            if (i_ >= payload_.size())
                THROW(SequenceError()) << "Integer expected, but EoF found in sequence " << payload_;
            if (payload_[i_] == ';')
                THROW(SequenceError()) << "Integer expected, but only sign found " << payload_;
        }
        bool ok = false;
        while (i_ < payload_.size()) {
            if (payload_[i_] == ';') {
                ++i_;
                break;
            }
            if (! helpers::IsDecimalDigit(payload_[i_]))
                THROW(SequenceError()) << "Expected number, but " << payload_[i_] << " found in sequence " << payload_;
            result = result * 10 + helpers::DecCharToNumber(payload_[i_++]);
            ok = true;
        }
        if (! ok)
            THROW(SequenceError()) << "Empty integer value not allowed " << payload_;
        return result * multiplier;
    }

    size_t Sequence::ContentsParser::parseUnsigned() {
        if (i_ >= payload_.size())
            THROW(SequenceError()) << "Unsigned expected, but EoF found in sequence " << payload_;
        size_t result = 0;
        bool ok = false;
        while (i_ < payload_.size()) {
            if (payload_[i_] == ';') {
                ++i_;
                break;
            }
            if (! helpers::IsDecimalDigit(payload_[i_]))
                THROW(SequenceError()) << "Expected number, but " << payload_[i_] << " found in sequence " << payload_;
            result = result * 10 + helpers::DecCharToNumber(payload_[i_++]);
            ok = true;
        }
        if (! ok)
            THROW(SequenceError()) << "Empty integer value not allowed " << payload_;
        return result;
    }

    std::string Sequence::ContentsParser::parseString() {
        if (i_ >= payload_.size())
            THROW(SequenceError()) << "string expected, but EoF found in sequence " << payload_;
        size_t nextSemicolon = payload_.find(';', i_);
        if (nextSemicolon == std::string::npos) {
            std::string result = payload_.substr(i_);
            i_ = payload_.size();
            return result;
        } else {
            std::string result = payload_.substr(i_, nextSemicolon - i_);
            i_ = nextSemicolon + 1;
            return result;
        }
    }

    std::string Sequence::ContentsParser::parseEncodedData() {
        std::string result{std::move(payload_)};
        char const * r = result.c_str() + i_;
        size_t w = 0;
        char const * e = result.c_str() + result.size();
        while (r < e) {
            result[w++] = Terminal::Decode(r, e);
        }
        result.resize(w);
        i_ = 0;
        return result;
    }

    void Sequence::ContentsParser::checkEoF() {
        if (i_ != payload_.size())
            THROW(SequenceError()) << "expected end of sequence"; 
    }

} // namespace tpp

#endif