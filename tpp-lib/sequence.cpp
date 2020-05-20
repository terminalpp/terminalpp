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
                THROW(helpers::IOError()) << "Expected decimal digit, but " << *start << " found in sequence payload";
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
        s << ';';
        WriteString(s, request_);
        s << ';' << id_;
    }

    // Sequence::Nack

    void Sequence::Nack::writeTo(std::ostream & s) const {
        Sequence::writeTo(s);
        s << ';';
        WriteString(s, request_);
        s << ';';
        WriteString(s, reason_);
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
