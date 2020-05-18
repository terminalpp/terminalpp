#include <cstdlib>
#include <iostream>
#include <fstream>
#include <memory>
#include <filesystem>

#include "helpers/helpers.h"
#include "helpers/filesystem.h"
#include "helpers/json_config.h"

#include "tpp-lib/local_pty.h"
#include "tpp-lib/terminal_client.h"

namespace tpp {

    using Log = helpers::Log;

    class Config : public helpers::JSONConfig::Root {
    public:
        CONFIG_OPTION(
            timeout,
            "Timeout of the connection to terminal++ (in ms)",
            "1000",
            unsigned
        );
        CONFIG_OPTION(
            adaptiveSpeed,
            "Adaptive speed",
            "true",
            bool
        );
        CONFIG_OPTION(
            packetSize,
            "Size of single packet of data",
            "1024",
            unsigned
        );
        CONFIG_OPTION(
            packetLimit,
            "Number of packets that can be sent without waiting for acknowledgement",
            "32",
            unsigned
        );
        CONFIG_OPTION(
            filename, 
            "Local file to be opened on the remote machine",
            "\"\"",
            std::string
        );
        CONFIG_OPTION(
            verbose,
            "Verbose output",
            "false",
            bool
        );

        static Config & Instance() {
            static Config singleton{};
            return singleton;
        }

        static Config & Setup(int argc, char * argv[]) {
            Config & result = Instance();
            helpers::JSONArguments args{};
            args.addArgument("Timeout", {"--timeout", "-t"}, result.timeout);
            args.addArgument("Packet size", {"--packet-size"}, result.packetSize);
            args.addArgument("Verbosity", {"--verbose", "-v"}, result.verbose);
            args.addArgument("Adaptive Speed", {"--adaptive"}, result.adaptiveSpeed);
            args.addArgument("Filename", {"--file", "-f"}, result.filename, /* expects value */ true, /* required */ true);
            args.setDefaultArgument("Filename");
            args.parse(argc, argv);
            return result;
        }

    private:

        Config() {
            initializationDone();
        }

    }; // tpp::Config

    class RemoteOpen {
    public:

        static constexpr size_t MIN_PACKET_LIMIT = 8;

        static void Transfer(TerminalClient::Sync & t, std::string const & filename) {
            RemoteOpen r{t, Config::Instance()};
            //r.openLocalFile(filename);
            r.transfer();
        }

    private:

        RemoteOpen(TerminalClient::Sync & t, Config const & config):
            t_{t},
            adaptiveSpeed_{config.adaptiveSpeed()},
            packetSize_{config.packetSize()},
            packetLimit_{config.packetLimit()},
            initialPacketLimit_{packetLimit_} {
            // verify the t++ capabilities of the terminal
            Sequence::Capabilities capabilities{t_.getCapabilities()};
            if (capabilities.version() != 1)
                THROW(helpers::Exception()) << "Incompatible t++ version " << capabilities.version() << " (required version 1)";
        }

        void openLocalFile(std::string const & filename) {
            try {
                std::string remoteHost = helpers::GetHostname();
                LOG(Log::Verbose) << "Remote host: " << remoteHost;
                std::string remoteFile = std::filesystem::canonical(filename);
                LOG(Log::Verbose) << "Remote file canonical path: " << remoteFile;
                f_.open(remoteFile);
                if (!f_.good())
                    throw false;
                f_.seekg(0, std::ios_base::end);
                size_ = f_.tellg();
                LOG(Log::Verbose) << "    size: " << size_;
                streamId_ = t_.openFileTransfer(remoteHost, remoteFile, size_);
                LOG(Log::Verbose) << "Assigned stream id: " << streamId_;
            } catch (...) {
                THROW(helpers::IOError()) << "Unable to open file " << filename;
            }
        }

        void transfer() {
            std::unique_ptr<char> buffer{new char[packetSize_]};
            LOG(Log::Verbose) << "Transferring, packet limit: " << packetLimit_;
            f_.seekg(0, std::ios_base::beg);
            sent_ = 0;
            size_t packets = 0;
            while (sent_ != size_) {
                f_.read(buffer.get(), packetSize_);
                size_t pSize = f_.gcount();
                // TODO this is a memory copy, can be done more effectively by having a non-ownership version of the Data message. 
                Sequence::Data d{streamId_, sent_, buffer.get(), buffer.get() + pSize};
                t_.send(d);
                sent_ += pSize;
                if (++packets == packetLimit_) {
                    packets = 0;
                    checkTransferStatus();
                }
                progressBar();
            }
        }

        void checkTransferStatus() {
            // TODO loop for retries, maybe handle this at the terminal client level
            Sequence::TransferStatus ts{t_.getTransferStatus(streamId_)};
            if (ts.received() == sent_) {
                if (adaptiveSpeed_ && packetLimit_ < initialPacketLimit_) {
                    packetLimit_ <<= 2;
                    LOG(Log::Verbose) << "Packet limit increased to " << packetLimit_; 
                }
            } else {
                LOG(Log::Verbose) << "Mismatch: sent " << sent_ << ", received " << ts.received();
                sent_ = ts.received();
                f_.clear();
                f_.seekg(sent_);
                if (adaptiveSpeed_ && (packetLimit_ > MIN_PACKET_LIMIT)) {
                    packetLimit_ >>= 1;
                    LOG(Log::Verbose) << "Packet limit decreased to " << packetLimit_; 
                }
            }
        }

        void progressBar() {
        }

        TerminalClient::Sync & t_;
        std::ifstream f_;
        size_t size_;
        size_t sent_;
        size_t streamId_;
        bool adaptiveSpeed_;
        size_t packetSize_;
        size_t packetLimit_;
        size_t initialPacketLimit_;
    }; 

} // namespace tpp



int main(int argc, char * argv[]) {
    using namespace tpp;
    try {
		helpers::Logger::Enable(
            helpers::Logger::StdOutWriter().setDisplayLocation(false).setDisplayName(false).setDisplayTime(false).setEoL("\r\n"), { 
                helpers::Log::Default(),
                helpers::Log::Verbose(),
                helpers::Log::Debug()
		});
        TerminalClient::Sync t{new LocalPTYSlave{}};
        RemoteOpen::Transfer(t, "tpp-lib/sequence.h");
        /*
        Sequence::Capabilities capabilities{t.getCapabilities()};
        LOG() << "t++ version " << capabilities.version() << " detected";
        size_t channel = t.openFileTransfer("host","someFile",1024);
        LOG() << "channel: " << channel;
        */
        return EXIT_SUCCESS;
    } catch (TimeoutError const & e) {
        std::cerr << "t++ terminal timeout." << std::endl;
    } catch (std::exception const & e) {
        std::cout << "\r\n Error: " << e.what() << "\r\n";
    } catch (...) {
        std::cout << "\r\n Unspecified errror.\r\n";
    }
    return EXIT_FAILURE;
}



#ifdef HAHA

#include "tpp-lib/terminal.h"

namespace tpp {

    class Config : public helpers::JSONConfig::Root {
    public:
        CONFIG_OPTION(
            timeout,
            "Timeout of the connection to terminal++ (in ms)",
            "1000",
            unsigned
        );
        CONFIG_OPTION(
            packetSize,
            "Size of single packet of data",
            "1024",
            unsigned
        );
        CONFIG_OPTION(
            filename, 
            "Local file to be opened on the remote machine",
            "\"\"",
            std::string
        );

        Config(int argc, char * argv[]) {
            initializationDone();
            helpers::JSONArguments args{};
            args.addArgument("Timeout", {"--timeout", "-t"}, timeout);
            args.addArgument("Packet size", {"--packet-size"}, packetSize);
            args.addArgument("Filename", {"--file", "-f"}, filename, /* expects value */ true, /* required */ true);
            args.setDefaultArgument("Filename");
            args.parse(argc, argv);
        }

    }; // Config

    class RemoteOpen {
    public:
        static constexpr size_t GOOD_PACKET_LIMIT = 128;
        static constexpr size_t BAD_PACKET_LIMIT = 8;

        static void Open(Terminal & terminal, std::string const & localFile, Config const & config) {
            RemoteOpen o{terminal, localFile, config};
            o.checkCapabilities();
            o.initialize();
            o.transfer();
            o.open();
        }

    private:
        RemoteOpen(Terminal & t, std::string const & localFile, Config const & config):
            t_(t),
            localFile_(localFile),
            packetSize_(config.packetSize()),
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
        Config config(argc, argv);
        StdTerminal t;
        t.setTimeout(config.timeout());
        tpp::RemoteOpen::Open(t, config.filename(), config);
        std::cout << "\n";
        return EXIT_SUCCESS;
    } catch (std::exception const & e) {
        std::cout << "\r\n Error: " << e.what() << "\r\n";
    } catch (...) {
        std::cout << "\r\n Unspecified errror.\r\n";
    }
    return EXIT_FAILURE;
}


#endif