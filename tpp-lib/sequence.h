#pragma once

#include <iostream>

#include "helpers/helpers.h"
#include "helpers/buffer.h"
#include "helpers/char.h"

namespace tpp {

    using Char = helpers::Char;
    using Buffer = helpers::Buffer;

    class PTYBase;

    /** Terminalpp Sequence base class. 

         - any extra payload is ignored so that it can be added in newer versions
     */
    class Sequence {
    public:
        enum class Kind {
            Ack = 0,
            Nack,
            /** Requests the terminal to send its capabilities. 
             */
            GetCapabilities,
            /** Describes the capabilities of the terminal, such as protocol version, and channels. 
             */
            Capabilities,
            Data,
            OpenFileTransfer,
            GetTransferStatus,
            TransferStatus,
            ViewRemoteFile,

            Invalid,
        };

        virtual ~Sequence() {
        }

        Kind kind() const {
            return kind_;
        }

        static char const * FindSequenceStart(char const * buffer, char const * bufferEnd);

        static char const * FindSequenceEnd(char const * buffer, char const * bufferEnd);

        /** Parses the sequence kind from a buffer and advances the reading position. 
         
            The seuence kind is a decimal number followed by either `;` to signify beginning of the sequence payload, or `BEL` to signify the end of the t++ sequence. 

            If the buffer does not contain enough information to determine the sequence kind, advances the reading position to the buffer end and returns Kind::Invalid. If invalid characters are found in the buffer, returns Kind::Invalid and advances the position to the invalid character.  
         */
        static Kind ParseKind(char const * & buffer, char const * bufferEnd);

        class Ack;
        class Nack;
        class GetCapabilities;
        class Capabilities;
        class Data;

        class OpenFileTransfer;
        class GetTransferStatus;
        class TransferStatus;
        class ViewRemoteFile;

    protected:

        Sequence(Kind kind):
            kind_{kind} {
        }

/*
        Sequence(char const * & start, char const * end, Kind expectedKind = Kind::Invalid) {
            size_t kind = ReadUnsigned(start, end);
            if (kind > static_cast<unsigned>(Kind::Invalid))
                THROW(SequenceError()) << "Invalid sequence kind " << kind;
            kind_ = static_cast<Kind>(kind);
            if (expectedKind != Kind::Invalid && (kind_ != expectedKind))
                THROW(SequenceError()) << "Expected sequence " << expectedKind << ", but found " << kind_;
        }
        */

        virtual void writeTo(std::ostream & s) const;

        Kind kind_;

        /** Reads unsigned value from the payload and moves the payload start past its end. 
         */
        static size_t ReadUnsigned(char const * & start, char const * end);

        static std::string ReadString(char const * & start, char const * end);

        static void WriteString(std::ostream & s, std::string const & vstr);

        /** Encodes the given buffer. 
         */
        static void Encode(std::ostream & s, char const * buffer, char const * end);

        /** Decodes the given buffer. 
         */
        static void Decode(Buffer & into, char const * buffer, char const * end);

    private:

        static char DecodeChar(char const * & x, char const * end) {
            if (*x == '`') {
                if (x + 3 > end)
                    THROW(helpers::IOError()) << "quote must be followed by 2 hexadecimal characters";
                char result = static_cast<char>(Char::ParseHexadecimalDigit(x[1]) * 16 + Char::ParseHexadecimalDigit(x[2]));
                x += 3;
                return result;
            } else {
                return *(x++);
            }
        }

        friend std::ostream & operator << (std::ostream & s, Sequence const & seq) {
            seq.writeTo(s);
            return s;
        }

    }; // tpp::Sequence

    std::ostream & operator << (std::ostream & s, Sequence::Kind kind);

    /** Acknowledgement. 
     */
    class Sequence::Ack : public Sequence {
    public:
        explicit Ack(size_t id):
            Sequence{Kind::Ack},
            request_{},
            id_{id} {
        }

        Ack(Sequence const & req, size_t id):
            Sequence{Kind::Ack},
            request_{STR(req)}, 
            id_{id} {
        }

        Ack(char const * & start, char const * end):
            Sequence(Kind::Ack) {
            request_ = ReadString(start, end);
            id_ = ReadUnsigned(start, end);
        }

        std::string const & request() const {
            return request_;
        }

        size_t id() const {
            return id_;
        }

    protected:

        void writeTo(std::ostream & s) const override;

    private:
        std::string request_;
        size_t id_;
    };

    /** Negative acknowledgement.
     */
    class Sequence::Nack : public Sequence {
    public:
        Nack(Sequence const & req, std::string const & reason):
            Sequence{Kind::Nack},
            request_{STR(req)}, 
            reason_{reason} {
        }

        Nack(char const * & start, char const * end):
            Sequence(Kind::Nack) {
            request_ = ReadString(start, end);
            reason_ = ReadString(start, end);
        }

        std::string const & request() const {
            return request_;
        }
        std::string const & reason() const {
            return reason_;
        }

    protected:

        void writeTo(std::ostream & s) const override;

    private:
        std::string request_;
        std::string reason_;

    }; // Sequence::Nack

    /** Terminal capabilities request. 
     */
    class Sequence::GetCapabilities : public Sequence {
    public:
        GetCapabilities():
            Sequence{Kind::GetCapabilities} {
        }

        GetCapabilities(char const * start, char const * end):
            Sequence{Kind::Capabilities} {
            MARK_AS_UNUSED(start);
            MARK_AS_UNUSED(end);
        }
    };

    /** Terminal capabilities information.
     */
    class Sequence::Capabilities : public Sequence {
    public:
        Capabilities(unsigned version):
            Sequence{Kind::Capabilities},
            version_{version} {
        }

        Capabilities(char const * start, char const * end):
            Sequence(Kind::Capabilities) {
            version_ = ReadUnsigned(start, end);
        }

        size_t version() const {
            return version_;
        }

    protected:

        void writeTo(std::ostream & s) const override;

    private:
        size_t version_;
    };

    /** Generic data transfer. 
     */
    class Sequence::Data : public Sequence {
    public:

        Data(size_t id, size_t packet, char const * payload, char const * payloadEnd):
            Sequence{Kind::Data},
            id_{id},
            packet_{packet},
            size_{static_cast<size_t>(payloadEnd - payload)},
            payload_{new char[size_]} {
            memcpy(payload_, payload, size_);
        }

        Data(size_t id, size_t packet, size_t size, std::istream & s):
            Sequence{Kind::Data},
            id_{id},
            packet_{packet},
            size_{size},
            payload_{new char[size_]} {
            s.read(payload_, size);
            size_ = s.gcount();
        }

        Data(char const * start, char const * end):
            Sequence{Kind::Data} {
            id_ = ReadUnsigned(start, end);
            packet_ = ReadUnsigned(start, end);
            size_ = ReadUnsigned(start, end);
            Buffer b;
            Decode(b, start, end);
            if (size_ != b.size())
                THROW(helpers::IOError()) << "Data Sequence size reported " << size_ << ", actual " << b.size();
            payload_ = b.release();
        }

        ~Data() override {
            delete [] payload_;
        }

        /** Returns the stream id. 
         */
        size_t id() const {
            return id_;
        }

        /** Returns the packet number within the data transfer. 
         
            The implementation of the packet number depends on the actual transfer being completed via the Data sequence and can even be ignored. 
         */
        size_t packet() const {
            return packet_;
        }

        /** Returns the size of the transferred data (payload). 
         */
        size_t size() const {
            return size_;
        }

        /** The actual paylod of the data transfer. 
         */
        char const * payload() const {
            return payload_;
        }

    protected:

        void writeTo(std::ostream & s) const override;

        //void sendTo(PTYBase & pty) const override;

    private:
        size_t id_;
        size_t packet_;
        size_t size_;
        char * payload_;
    }; // Sequence::Data

    class Sequence::OpenFileTransfer : public Sequence {
    public:

        OpenFileTransfer(std::string const & host, std::string const & filename, size_t fileSize):
            Sequence{Kind::OpenFileTransfer},
            remoteHost_{host},
            remotePath_{filename},
            size_{fileSize} {
        }

        OpenFileTransfer(char const * start, char const * end):
            Sequence{Kind::OpenFileTransfer} {
            remoteHost_ = ReadString(start, end);
            remotePath_ = ReadString(start, end);
            size_ = ReadUnsigned(start, end);
        }

        std::string const & remoteHost() const {
            return remoteHost_;
        }

        std::string const & remotePath() const {
            return remotePath_;
        }

        size_t size() const {
            return size_;
        }

    protected:

        void writeTo(std::ostream & s) const override;

    private:
        std::string remoteHost_;
        std::string remotePath_;
        size_t size_;

    }; // Sequence::OpenFileTransfer

    /** Returns the status of a transferred file. 
     */
    class Sequence::GetTransferStatus : public Sequence {
    public:

        GetTransferStatus(size_t id):
            Sequence{Kind::GetTransferStatus},
            id_{id} {
        }

        GetTransferStatus(char const * & start, char const * end):
            Sequence(Kind::GetTransferStatus) {
            id_ = ReadUnsigned(start, end);
        }

        size_t id() const {
            return id_;
        }

    protected:

        void writeTo(std::ostream & s) const override;

    private:
        size_t id_;

    }; // Sequence::GetTransferStatus

    class Sequence::TransferStatus : public Sequence {
    public:

        TransferStatus(size_t id, size_t size, size_t received):
            Sequence{Kind::TransferStatus},
            id_{id},
            size_{size},
            received_{received} {
        }

        TransferStatus(char const * & start, char const * end):
            Sequence(Kind::TransferStatus) {
            id_ = ReadUnsigned(start, end);
            size_ = ReadUnsigned(start, end);
            received_ = ReadUnsigned(start, end);
        }

        size_t id() const {
            return id_;
        }

        size_t size() const {
            return size_;
        }

        size_t received() const {
            return received_;
        }

    protected:

        void writeTo(std::ostream & s) const override;

    private:
        size_t id_;
        size_t size_;
        size_t received_;

    }; // Sequence::TransferStatus

    /** Opens remote file in view mode.
     */
    class Sequence::ViewRemoteFile : public Sequence {
    public:

        ViewRemoteFile(size_t id):
            Sequence{Kind::ViewRemoteFile},
            id_{id} {
        }

        ViewRemoteFile(char const * & start, char const * end):
            Sequence(Kind::ViewRemoteFile) {
            id_ = ReadUnsigned(start, end);
        }

        size_t id() const {
            return id_;
        }

    protected:

        void writeTo(std::ostream & s) const override;

    private:
        size_t id_;

    }; // Sequence::ViewRemoteFile

} // namespace tpp
