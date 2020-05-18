#pragma once

#include <iostream>

#include "helpers/helpers.h"
#include "helpers/buffer.h"
#include "helpers/char.h"

namespace tpp {

    using Char = helpers::Char;
    using Buffer = helpers::Buffer;

    class PTYBase;

    class SequenceError : public helpers::Exception {
    };

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
/*
        class FileTransfer;
        class GetTransferStatus;
        class TransferStatus;
    */

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
            id_{id},
            payload_{} {
        }

        Ack(size_t id, std::string const & payload):
            Sequence{Kind::Ack},
            id_{id},
            payload_{payload} {
        }

        Ack(size_t id, Sequence const & req):
            Sequence{Kind::Ack},
            id_{id},
            payload_{STR(req)} {
        }

        Ack(char const * & start, char const * end):
            Sequence(Kind::Ack) {
            id_ = ReadUnsigned(start, end);
            payload_ = ReadString(start, end);
        }

        size_t id() const {
            return id_;
        }

        std::string const & payload() const {
            return payload_;
        }

    protected:

        void writeTo(std::ostream & s) const override;

    private:
        size_t id_;
        std::string payload_;
    };

    class Sequence::Nack : public Sequence {

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
            if (size_ == b.size())
                THROW(helpers::IOError()) << "Data Sequence size reported " << size_ << ", actual " << b.size();
            payload_ = b.release();
        }

        ~Data() override {
            delete payload_;
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

    /** Requests the capabilities from the server. 
     */
    /*
    class Sequence::GetCapabilities : public Sequence {
    public:
        GetCapabilities():
            Sequence{Kind::GetCapabilities} {
        }
    protected:

        static constexpr Kind Kind_ = Kind::GetCapabilities;

        GetCapabilities(char const * payloadStart, char const * payloadEnd):
            Sequence{Kind::GetCapabilities} {
            if (payloadStart != payloadEnd)
                THROW(SequenceError()) << "Unexpected extra paylod for " << Kind_;
        }
    }; 
    */

    /** Server capabilities. 
     */
    /*
    class Sequence::Capabilities : public Sequence {
    public:
        Capabilities(unsigned version):
            Sequence{Kind::Capabilities},
            version_{version} {
        }

        unsigned version() const {
            return version_;
        }

    protected:

        static constexpr Kind Kind_ = Kind::Capabilities;

        Capabilities(char const * payloadStart, char const * payloadEnd):
            Sequence{Kind::Capabilities, payloadStart, payloadEnd} {
            version_ = readUnsigned();
        }

        void sendTo(TerminalPTY & pty) const override;
    private:
        unsigned version_;
    };
    */

} // namespace tpp

#ifdef HAHA

#include <string>

#include "helpers/helpers.h"

/** \page tppseqences Terminal++ Sequences
 
    \brief Reference of the extra `t++` sequences used. 

    `t++` supports extra sequences that can be used to both utilise the extra graphical features of the terminal and to support more advanced connections. The extra graphical features take the form of extra `SGR` and similar escape sequences, while the advanced connection features all use the `DCS` escape prefix followed by `+`, i.e. `\033P+` and must be terminated by the `BEL` character. 

    The tpp::Sequence class implements the basic reading of the advanced connection sequence and provides subclasses that further parse the generic `t++` `DSC` sequence into specific request or response sequences. The sections below provide details on these:

    \section tppseqcaps `t++` Capabilities

    The tpp::Sequence::Kind::Capabilities request asks the terminal for the supported protocol version and the associated response returns it. It is intended as a check for the presence of a `terminal++`. 

    > The current version is 0.  

    \section tppremotefiles Remote Files Protocol

    The terminal supports sending files local to the connected machine to the terminal's pc where they are stored in a local folder and can then be processed locally. This functionality is achieved via the connections file API described here. 

    > The connections API is a bit more general than that as in the future, it might be used to do more advanced things, such as to update the files when needed, edit them, or even multiplex multiple communication channels in a single terminal connection, which is why the file id's are used even though the code now only works properly with single file used at the time. 

    tpp::Sequence::Kind::NewFile

    First, new file id for the connection must be obtained via the `NewFile`, which takes the size of the file, hostname, filename and local path of the file as arguments. The terminal stores this information, creates the temporary file and returns the id of the connection to be used for the transfer. 

    tpp::Sequence::Kind::Data

    Data is transferred in chunks via the `Data` request. Each data request takes the connection id, the size of the transmitted data and the offset of that data block from the beginning of the transferred file, followed by the encoded data block (see the \ref tppseqencoding "encoding" section below).

    > The offsets and size are provided for redundancy so that split messages can be detected and repaired as `tmux` (and possibly others) mangle the sequences in some cases, which is outside of the scope of `t++` to fix. 

    tpp::Sequence::Kind::TransferStatus

    The `Data` messages are not associated with any response, but the transmitted may explicitly request the transfer status of a file at any point with the file id. The response message contains the information about the number of bytes from the beginning of the file that were correctly transmitted. It is assumed that upon receiving the response, the client will adjust accordingly so that next data messages will follow right after the correctly transmitted data. 

    > The `TransferStatus` sequence should be used reasonably often during the transmission so that transmission buffers of any programs in the middle (`tmux`, etc.) can be flushed.     

    tpp::Sequence::Kind::OpenFile

    Once the file is transmitted entirely, the `OpenFile` instructs the terminal to open the file using the terminal's system local viewer. Once the message is received, the `Ack` message is sent back and should be accepted by the client.

    \section tppseqencoding `t++` Encoding Scheme

    Data sent via the `t++` protocol in `DCS` sequences is encoded with a simple scheme so that it does not interfere with the terminal escape sequences layer. The encoding explicitly prohibits the `ESC` and `BEL` characters to appear unencoded. Whenever a backtick character is present, it must be followed by 2 hex digits which encode the character to be used. The backtick itself is encoded in the same scheme, i.e. hex number `60`. 
 */

namespace tpp {

    class SequenceError : public helpers::Exception {
    };

    /** A Terminal++ escape sequence. 
     
        Extra `tpp` sequences are implemented as special Device Control (DCS) sequences that start with `\033P+` and end with the `BEL` character.

     */
    class Sequence {
    public:

        enum class Kind : int {
            Invalid = -2, 
            Incomplete = -1, 
            Ack = 0, 
            Capabilities,
            NewFile,
            Data,
            TransferStatus,
            OpenFile
        }; 

        class AckResponse;
        class CapabilitiesRequest;
        class CapabilitiesResponse;
        class NewFileRequest;
        class NewFileResponse;
        class DataRequest;
        class TransferStatusRequest;
        class TransferStatusResponse;
        class OpenFileRequest;

        Sequence(Sequence && from):
            id_(from.id_),
            payload_(std::move(from.payload_)) {
        }

        bool complete() const {
            return id_ != Kind::Incomplete;
        }

        bool valid() const {
            return static_cast<int>(id_) >= 0;
        }

        Kind id() const {
            return id_;
        }

        std::string const & payload() const {
            return payload_;
        }

        static Sequence Parse(char * & start, char const * end); 

    protected:

        Kind id_;
        std::string payload_;

    private:

        friend class Terminal;

        class ContentsParser;

        Sequence():
            id_{Kind::Invalid},
            payload_{} {
        }

        Sequence(Kind id, std::string && payload):
            id_{id},
            payload_(std::move(payload)) {
        }

        friend std::ostream & operator << (std::ostream & s, Sequence const & seq) {
            s << "T++ sequence id " << static_cast<int>(seq.id()) << ", payload: " << seq.payload();
            return s;
        } 
    };

    class Sequence::AckResponse {
    public:
        AckResponse(Sequence && seq);
    };

    class Sequence::CapabilitiesRequest {
    public:
        CapabilitiesRequest(Sequence && seq);

    };

    class Sequence::CapabilitiesResponse {
    public:
        CapabilitiesResponse(Sequence && seq);

        int version;
    };

    class Sequence::NewFileRequest {
    public:
        NewFileRequest(Sequence && seq);

        size_t size;
        std::string hostname;
        std::string filename;
        std::string remotePath;

    };

    class Sequence::NewFileResponse {
    public:
        NewFileResponse(Sequence && seq);
        NewFileResponse():
            fileId{-1} {
        }

        int fileId;

    };

    class Sequence::DataRequest {
    public:
        DataRequest(Sequence && seq);

        int fileId;
        size_t offset;
        std::string data;
    };

    class Sequence::TransferStatusRequest {
    public:
        TransferStatusRequest(Sequence && seq);
        int fileId;

    };

    class Sequence::TransferStatusResponse {
    public:
        TransferStatusResponse(Sequence && seq);
        TransferStatusResponse():
            fileId{-1},
            transmittedBytes{0} {
        }

        int fileId;
        size_t transmittedBytes;
    };

    class Sequence::OpenFileRequest {
    public:
        OpenFileRequest(Sequence && seq);

        int fileId;
    };

    /** Parses the payload of a generic sequence into fragments needed by the concrete sequence types. 
     */
    class Sequence::ContentsParser {
    public:
        ContentsParser(Sequence && seq):
            i_{0},
            payload_{std::move(seq.payload_)} {
        }

        /** Reads an integer value. 
         
            Decimal digits optionally preceded with a single negative sign are allowed. Empty numbers are not allowed. 
         */
        int parseInt();

        /** Reads an unsigned value.
         
            Decimal digits are allowed. 
         */
        size_t parseUnsigned();

        /** Parses a string. 
         
            String is delimited by a semicolon and currently may not contain semicolons in it. 
         */
        std::string parseString();

        /** Parses the rest of the payload as encoded data. 
         */
        std::string parseEncodedData();

        /** Verifies that the whole payload has been processed. 
         */
        void checkEoF();

    private:
        size_t i_;
        std::string payload_;
    };

    inline std::ostream & operator << (std::ostream & s, Sequence::Kind kind) {
        s << static_cast<int>(kind);
        return s;
    }

} // namespace tpp

#endif