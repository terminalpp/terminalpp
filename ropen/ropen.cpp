#include <cstdlib>
#include <iostream>
#include <fstream>
#include <memory>

#include "helpers/helpers.h"
#include "helpers/args.h"

#include "tpp-lib/terminal.h"

namespace tpp {

    helpers::Arg<unsigned> Timeout {
        { "--timeout", "-t"},
        1000,
        false,
        "Timeout of the connection to terminal++ (in ms)"
    };

    helpers::Arg<unsigned> PacketSize {
        { "--packet-size" },
        1024,
        false,
        "Size of single packet of data"
    };

    /*
    helpers::Arg<bool> Verbose {
        { "--verbose", "-v" },
        false,
        false,
        "Logs all kinds of details when on"
    };
    */

    helpers::Arg<std::string> Filename{
        { "--file", "-f" },
        "",
        true,
        "File to be opened on the remote machine"
    };

    class RemoteOpen {
    public:
        static constexpr size_t GOOD_PACKET_LIMIT = 128;
        static constexpr size_t BAD_PACKET_LIMIT = 8;

        static void Open(Terminal & terminal, std::string const & localFile) {
            RemoteOpen o{terminal, localFile};
            o.checkCapabilities();
            o.initialize();
            o.transfer();
            o.open();
        }

    private:
        RemoteOpen(Terminal & t, std::string const & localFile):
            t_(t),
            localFile_(localFile),
            packetSize_(*PacketSize),
            packets_{0},
            packetLimit_{GOOD_PACKET_LIMIT},
            totalBytes_{0},
            sentBytes_{0},
            barWidth_{40} {
        }

        void checkCapabilities() {
            try {
                Sequence::CapabilitiesResponse cap{t_.getCapabilities()};
            } catch (SequenceError const & e) {
                THROW(helpers::IOError()) << "Unable to connect to terminal++";
            }
        }

        /** Initializes the transmission and obtains the file id. 
         */    
        void initialize() {
            s_.open(localFile_);
            if (! s_.good())
                THROW(helpers::IOError()) << "Unable to open local file " << localFile_;
            s_.seekg(0, std::ios_base::end);
            totalBytes_ = s_.tellg();
            s_.seekg(0, std::ios_base::beg);

            fileId_ = t_.newFile(localFile_, totalBytes_).fileId;
        }

        void transfer() {
            std::unique_ptr<char> buffer{new char[packetSize_]};
            sentBytes_ = 0;
            packets_ = 0;
            while (sentBytes_ != totalBytes_) {
                s_.read(buffer.get(), packetSize_);
                size_t pSize = s_.gcount();
                t_.transmit(fileId_, sentBytes_, buffer.get(), pSize);
                sentBytes_ += pSize;
                checkTransferStatus();
                progressBar();
            }
        }

        void open() {
            t_.openFile(fileId_);
        }

        void checkTransferStatus() {
            if (packets_++ == packetLimit_ || sentBytes_ == totalBytes_) {
                // TODO how many retrials we want
                for (size_t i = 0; i < 5; ++i) {
                    try {
                        size_t transmitted = t_.transferStatus(fileId_).transmittedBytes;
                        if (transmitted != sentBytes_) {
                            // decrease packet limit
                            if (packetLimit_ > 8)
                                packetLimit_ >>= 1;
                            sentBytes_ = transmitted;
                            s_.clear();
                            s_.seekg(sentBytes_);
                        } else if (packetLimit_ < GOOD_PACKET_LIMIT) {
                            packetLimit_ <<= 2;
                        }
                        packets_ = 0;
                        return;
                    } catch (helpers::TimeoutError const &) {
                        continue;
                    } catch (...) {
                        throw;
                    }
                }
            }
        }

        void progressBar() {
            size_t pct = sentBytes_ * 100 / totalBytes_;
            size_t barX = barWidth_ * pct / 100;
            if (packetLimit_ >= GOOD_PACKET_LIMIT)
                std::cout << "[\033[32m";
            else if (packetLimit_ >= BAD_PACKET_LIMIT)
                std::cout << "[\033[33m";
            else 
                std::cout << "[\033[91m";
            for (size_t i = 0; i < barX; ++i)
                std::cout <<  '#';
            std::cout << "\033[0m";
            for (size_t i = barX; i < barWidth_; ++i)
                std::cout <<  ' ';
            std::cout <<  "] " << pct << "% \r" << std::flush;
        }

        Terminal & t_;
        std::string const & localFile_;
        std::ifstream s_;
        int fileId_;
        size_t packetSize_;
        size_t packets_;
        size_t packetLimit_;
        size_t totalBytes_;
        size_t sentBytes_;

        size_t barWidth_;

    };

} // namespace tpp

int main(int argc, char * argv[]) {
    using namespace tpp;
    try {
        helpers::Arguments::SetDefaultArgument(Filename);
        helpers::Arguments::SetVersion(PROJECT_VERSION);
        helpers::Arguments::Parse(argc, argv);
        StdTerminal t;
        t.setTimeout(*Timeout);
        tpp::RemoteOpen::Open(t, *Filename);
        std::cout << "\n";
        return EXIT_SUCCESS;
    } catch (std::exception const & e) {
        std::cout << "\r\n Error: " << e.what() << "\r\n";
    } catch (...) {
        std::cout << "\r\n Unspecified errror.\r\n";
    }
    return EXIT_FAILURE;
}